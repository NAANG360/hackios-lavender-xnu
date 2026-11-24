/*
 * LavenderGPU: the Adreno 512 for programs to use.
 *
 * lavender_gpu.c wakes the GPU and starts the command processor (the ladder up to step 12, boot-arg
 * lavgpu=30); when the ring is up it calls LavenderGPUStart() and this service appears. A program
 * opens it with IOServiceOpen() and speaks four requests (selectors of IOConnectCallStructMethod):
 *
 *   0 kLavInfo     -> struct lav_info: version, whether the GPU is ready, how many streams went in, last fence
 *   1 kLavBufNew   <- struct lav_buf_new {size}, -> {handle, address}: a buffer of physically contiguous memory,
 *                     zeroed, write-combined; address is what the GPU calls it (the GPU's translation of this
 *                     driver is the identity, so it is the physical address). The program maps it with
 *                     IOConnectMapMemory64(handle as the memory type).
 *   2 kLavBufFree  <- struct lav_buf_free {handle}
 *   3 kLavSubmit   <- an array of 32-bit PM4 words, -> struct lav_submit_out {fence, status}: the words go into the
 *                     ring followed by a cache flush and a fence write, and the call returns when the GPU has
 *                     reached the fence (it is synchronous: one stream at a time).
 *
 * Nothing here protects the system from a program that names addresses it does not own: this is the bring-up
 * interface (one trusted client, a root test tool or the Metal plug-in), not a security boundary.
 */

#include <IOKit/IOLib.h>
#include <IOKit/IOLocks.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <mach/vm_param.h>
#include <sys/systm.h>

extern "C" {
int lavender_gpu_service_ready(void);
uint32_t lavender_gpu_service_submit(const uint32_t * dwords, uint32_t count, int * status);
void LavenderGPUStart(void);
void LavenderGPUSelfTest(void);
}

enum {
	kLavInfo        = 0,
	kLavBufNew      = 1,
	kLavBufFree     = 2,
	kLavSubmit      = 3,
	kLavSubmitBuf   = 4,
	kLavMetal       = 5,
};

#define LAV_BUFFERS     64
#define LAV_BUFFER_MAX  (16ull << 20)
#define LAV_SUBMIT_MAX  16384           /* bytes of PM4 words in one stream */

struct lav_info {
	uint32_t version, ready, submits, last_fence;
};
struct lav_buf_new {
	uint64_t size;
	uint32_t handle, pad;
	uint64_t address;
};
struct lav_buf_free {
	uint32_t handle, pad;
};
struct lav_submit_out {
	uint32_t fence, status;
};
struct lav_submit_buf {
	uint32_t handle, offset, dwords, pad;
};

static IOLock * gLavLock;
static uint32_t gLavSubmits, gLavLast;

class LavenderGPU : public IOService
{
	OSDeclareDefaultStructors(LavenderGPU);

public:
	virtual bool start(IOService * provider) APPLE_KEXT_OVERRIDE;
};

OSDefineMetaClassAndStructors(LavenderGPU, IOService);

bool
LavenderGPU::start(IOService * provider)
{
	if (!IOService::start(provider)) {
		return false;
	}
	setProperty(kIOUserClientClassKey, "LavenderGPUUserClient");
	setName("LavenderGPU");
	registerService();
	return true;
}

void
LavenderGPUStart(void)
{
	IOService * provider = IOService::getPlatform();
	LavenderGPU * gpu;

	if (gLavLock == NULL) {
		gLavLock = IOLockAlloc();
	}
	gpu = OSTypeAlloc(LavenderGPU);
	if (gpu == NULL || provider == NULL || !gpu->init() || !gpu->attach(provider)) {
		OSSafeReleaseNULL(gpu);
		IOLog("LavenderGPU: cannot create the service\n");
		return;
	}
	if (!gpu->start(provider)) {
		gpu->detach(provider);
		IOLog("LavenderGPU: the service did not start\n");
	} else {
		IOLog("LavenderGPU: service started\n");
	}
	gpu->release();
}

/*
 * Metal finds its device through the IOKit service of class IOAcceleratorES (the empty one the framebuffer driver publishes):
 * it reads MetalPluginName and MetalPluginClassName from it and loads /System/Library/Extensions/<name>.bundle. The properties
 * are put there only when a client asks (and taken away again), so the rest of the system keeps drawing as it did.
 */
static IOReturn
lavMetalSet(uint32_t on)
{
	OSDictionary * match = IOService::serviceMatching("IOAcceleratorES");
	IOService * accel = match != NULL ? IOService::copyMatchingService(match) : NULL;

	if (match != NULL) {
		match->release();
	}
	if (accel == NULL) {
		IOLog("LavenderGPU: Metal: there is no IOAcceleratorES service\n");
		return kIOReturnNotFound;
	}
	if (on) {
		accel->setProperty("MetalPluginName", "LavenderMetal");
		accel->setProperty("MetalPluginClassName", "LavenderMetalDevice");
	} else {
		accel->removeProperty("MetalPluginName");
		accel->removeProperty("MetalPluginClassName");
	}
	IOLog("LavenderGPU: Metal: plug-in properties %s on IOAcceleratorES\n", on ? "set" : "removed");
	accel->release();
	return kIOReturnSuccess;
}

class LavenderGPUUserClient : public IOUserClient
{
	OSDeclareDefaultStructors(LavenderGPUUserClient);

	task_t                          fTask;
	IOBufferMemoryDescriptor *      fMemory[LAV_BUFFERS];   /* handle n is fMemory[n - 1] */
	uint64_t                        fAddress[LAV_BUFFERS];

	IOReturn bufferNew(struct lav_buf_new * request);
	IOReturn bufferFree(uint32_t handle);
	IOReturn submit(IOExternalMethodArguments * args, struct lav_submit_out * out);
	IOReturn submitBuf(const struct lav_submit_buf * request, struct lav_submit_out * out);
	IOReturn run(uint32_t * words, size_t size, struct lav_submit_out * out);

public:
	bool selfTest(void);
	virtual bool initWithTask(task_t owningTask, void * securityID, UInt32 type, OSDictionary * properties) APPLE_KEXT_OVERRIDE;
	virtual IOReturn clientClose(void) APPLE_KEXT_OVERRIDE;
	virtual IOReturn clientMemoryForType(UInt32 type, IOOptionBits * options, IOMemoryDescriptor ** memory) APPLE_KEXT_OVERRIDE;
	virtual IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments * args,
	    IOExternalMethodDispatch * dispatch, OSObject * target, void * reference) APPLE_KEXT_OVERRIDE;
};

OSDefineMetaClassAndStructors(LavenderGPUUserClient, IOUserClient);

bool
LavenderGPUUserClient::initWithTask(task_t owningTask, void * securityID, UInt32 type, OSDictionary * properties)
{
	if (!IOUserClient::initWithTask(owningTask, securityID, type, properties)) {
		return false;
	}
	fTask = owningTask;
	return true;
}

IOReturn
LavenderGPUUserClient::clientClose(void)
{
	IOLockLock(gLavLock);
	for (uint32_t i = 0; i < LAV_BUFFERS; i++) {
		if (fMemory[i] != NULL) {
			fMemory[i]->release();
			fMemory[i] = NULL;
		}
	}
	IOLockUnlock(gLavLock);
	if (!isInactive()) {
		terminate();
	}
	return kIOReturnSuccess;
}

IOReturn
LavenderGPUUserClient::clientMemoryForType(UInt32 type, IOOptionBits * options, IOMemoryDescriptor ** memory)
{
	IOReturn ret = kIOReturnBadArgument;

	IOLockLock(gLavLock);
	if (type >= 1 && type <= LAV_BUFFERS && fMemory[type - 1] != NULL) {
		fMemory[type - 1]->retain();
		*memory = fMemory[type - 1];
		*options = 0;
		ret = kIOReturnSuccess;
	}
	IOLockUnlock(gLavLock);
	return ret;
}

IOReturn
LavenderGPUUserClient::bufferNew(struct lav_buf_new * request)
{
	IOBufferMemoryDescriptor * memory;
	uint64_t size = round_page_64(request->size);
	uint32_t slot = LAV_BUFFERS;

	if (request->size == 0 || size > LAV_BUFFER_MAX) {
		return kIOReturnBadArgument;
	}
	/* below 4 GB and in one piece: the GPU's page table is the identity of the first four gigabytes */
	memory = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task,
	    kIODirectionInOut | kIOMemoryPhysicallyContiguous | kIOMapWriteCombineCache,
	    size, 0x00000000FFFFF000ULL);
	if (memory == NULL) {
		IOLog("LavenderGPU: no contiguous memory for a buffer of %llu bytes\n", size);
		return kIOReturnNoMemory;
	}
	IOLockLock(gLavLock);
	for (uint32_t i = 0; i < LAV_BUFFERS; i++) {
		if (fMemory[i] == NULL) {
			slot = i;
			break;
		}
	}
	if (slot == LAV_BUFFERS) {
		IOLockUnlock(gLavLock);
		memory->release();
		return kIOReturnNoResources;
	}
	fMemory[slot] = memory;
	fAddress[slot] = memory->getPhysicalSegment(0, NULL, kIOMemoryMapperNone);
	request->handle = slot + 1;
	request->address = fAddress[slot];
	request->size = size;
	IOLockUnlock(gLavLock);
	IOLog("LavenderGPU: buffer %u: pa %llx size %llu\n", request->handle, request->address, size);
	return kIOReturnSuccess;
}

IOReturn
LavenderGPUUserClient::bufferFree(uint32_t handle)
{
	IOReturn ret = kIOReturnBadArgument;

	IOLockLock(gLavLock);
	if (handle >= 1 && handle <= LAV_BUFFERS && fMemory[handle - 1] != NULL) {
		fMemory[handle - 1]->release();
		fMemory[handle - 1] = NULL;
		ret = kIOReturnSuccess;
	}
	IOLockUnlock(gLavLock);
	return ret;
}

/* The stream comes inline (up to 4 KB) or as a memory descriptor (larger); either way it is copied before use. */
IOReturn
LavenderGPUUserClient::submit(IOExternalMethodArguments * args, struct lav_submit_out * out)
{
	uint32_t * words;
	size_t size;

	if (args->structureInputDescriptor != NULL) {
		size = (size_t)args->structureInputDescriptor->getLength();
	} else {
		size = args->structureInputSize;
	}
	if (size == 0 || size > LAV_SUBMIT_MAX || (size & 3) != 0) {
		return kIOReturnBadArgument;
	}
	words = (uint32_t *)IOMalloc(size);
	if (words == NULL) {
		return kIOReturnNoMemory;
	}
	if (args->structureInputDescriptor != NULL) {
		if (args->structureInputDescriptor->readBytes(0, words, size) != size) {
			IOFree(words, size);
			return kIOReturnVMError;
		}
	} else {
		memcpy(words, args->structureInput, size);
	}
	return run(words, size, out);
}

IOReturn
LavenderGPUUserClient::run(uint32_t * words, size_t size, struct lav_submit_out * out)
{
	IOReturn ret = kIOReturnSuccess;
	int status = 0;

	IOLockLock(gLavLock);
	out->fence = lavender_gpu_service_submit(words, (uint32_t)(size / 4), &status);
	if (out->fence != 0) {
		gLavSubmits++;
		gLavLast = out->fence;
	}
	IOLockUnlock(gLavLock);
	IOLog("LavenderGPU: submit of %llu bytes: fence %u status %d, first words %08x %08x %08x %08x\n", (unsigned long long)size, out->fence, status, size >= 4 ? words[0] : 0, size >= 8 ? words[1] : 0, size >= 12 ? words[2] : 0, size >= 16 ? words[3] : 0);
	out->status = (uint32_t)status;
	if (status != 0) {
		ret = kIOReturnIOError;
	}
	IOFree(words, size);
	return ret;
}

/* The stream is already in one of the client's buffers: copy it out of there, which avoids IOKit's large-input path. */
IOReturn
LavenderGPUUserClient::submitBuf(const struct lav_submit_buf * request, struct lav_submit_out * out)
{
	uint32_t * words;
	size_t size = (size_t)request->dwords * 4;
	IOReturn ret = kIOReturnBadArgument;

	if (request->dwords == 0 || size > LAV_SUBMIT_MAX || (request->offset & 3) != 0) {
		return kIOReturnBadArgument;
	}
	words = (uint32_t *)IOMalloc(size);
	if (words == NULL) {
		return kIOReturnNoMemory;
	}
	IOLockLock(gLavLock);
	if (request->handle >= 1 && request->handle <= LAV_BUFFERS && fMemory[request->handle - 1] != NULL &&
	    (uint64_t)request->offset + size <= fMemory[request->handle - 1]->getLength()) {
		if (fMemory[request->handle - 1]->readBytes(request->offset, words, size) == size) {
			ret = kIOReturnSuccess;
		} else {
			ret = kIOReturnVMError;
		}
	}
	IOLockUnlock(gLavLock);
	if (ret != kIOReturnSuccess) {
		IOFree(words, size);
		return ret;
	}
	return run(words, size, out);
}

IOReturn
LavenderGPUUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments * args,
    IOExternalMethodDispatch * dispatch, OSObject * target, void * reference)
{
	union {
		struct lav_info info;
		struct lav_buf_new bufNew;
		struct lav_buf_free bufFree;
		struct lav_submit_out out;
	} r;
	size_t size = args->structureInputSize;
	IOReturn ret = kIOReturnSuccess;
	size_t outSize = 0;

	bzero(&r, sizeof(r));
	switch (selector) {
	case kLavInfo:
		r.info.version = 1;
		r.info.ready = lavender_gpu_service_ready() ? 1 : 0;
		r.info.submits = gLavSubmits;
		r.info.last_fence = gLavLast;
		outSize = sizeof(r.info);
		break;
	case kLavBufNew:
		if (args->structureInput == NULL || size < sizeof(uint64_t)) {
			return kIOReturnBadArgument;
		}
		memcpy(&r.bufNew, args->structureInput, size < sizeof(r.bufNew) ? size : sizeof(r.bufNew));
		ret = bufferNew(&r.bufNew);
		outSize = sizeof(r.bufNew);
		break;
	case kLavBufFree:
		if (args->structureInput == NULL || size < sizeof(uint32_t)) {
			return kIOReturnBadArgument;
		}
		memcpy(&r.bufFree, args->structureInput, size < sizeof(r.bufFree) ? size : sizeof(r.bufFree));
		ret = bufferFree(r.bufFree.handle);
		break;
	case kLavSubmit:
		if (!lavender_gpu_service_ready()) {
			return kIOReturnNotReady;
		}
		ret = submit(args, &r.out);
		outSize = sizeof(r.out);
		break;
	case kLavSubmitBuf:
		if (args->structureInput == NULL || size < sizeof(struct lav_submit_buf)) {
			return kIOReturnBadArgument;
		}
		if (!lavender_gpu_service_ready()) {
			return kIOReturnNotReady;
		}
		ret = submitBuf((const struct lav_submit_buf *)args->structureInput, &r.out);
		outSize = sizeof(r.out);
		break;
	case kLavMetal:
		if (args->structureInput == NULL || size < sizeof(uint32_t)) {
			return kIOReturnBadArgument;
		}
		ret = lavMetalSet(*(const uint32_t *)args->structureInput);
		break;
	default:
		IOLog("LavenderGPU: request %u is not known here\n", selector);
		return kIOReturnUnsupported;
	}
	if (args->structureOutput != NULL && outSize != 0) {
		if (args->structureOutputSize > outSize) {
			args->structureOutputSize = (uint32_t)outSize;
		}
		memcpy(args->structureOutput, &r, args->structureOutputSize);
	}
	return ret;
}

/* PM4 type 7 header: odd parity of the count and of the opcode, as the CP wants it. */
static uint32_t
lav_parity(uint32_t v)
{
	v ^= v >> 16;
	v ^= v >> 8;
	v ^= v >> 4;
	return (0x9669u >> (v & 0xf)) & 1u;
}

static uint32_t
lav_pkt7(uint32_t opcode, uint32_t count)
{
	return 0x70000000u | (count & 0x3fff) | (lav_parity(count) << 15) | ((opcode & 0x7f) << 16) | (lav_parity(opcode) << 23);
}

/*
 * What a program would do, from inside the kernel: open (initWithTask), ask for the state, make a buffer, have the
 * GPU write two words into it with CP_MEM_WRITE through a submitted stream, read them back, give the buffer back.
 */
bool
LavenderGPUUserClient::selfTest(void)
{
	IOExternalMethodArguments args;
	struct lav_info info;
	struct lav_buf_new bn;
	struct lav_buf_free bf;
	struct lav_submit_out out;
	uint32_t stream[8];
	volatile uint32_t * words;
	IOReturn ret;
	bool ok = false;

	bzero(&args, sizeof(args));
	bzero(&info, sizeof(info));
	args.structureOutput = &info;
	args.structureOutputSize = sizeof(info);
	ret = externalMethod(kLavInfo, &args, NULL, NULL, NULL);
	IOLog("LavenderGPU: selftest info: ret %x version %u ready %u submits %u last %u\n", ret, info.version, info.ready, info.submits, info.last_fence);

	bzero(&bn, sizeof(bn));
	bn.size = 4096;
	bzero(&args, sizeof(args));
	args.structureInput = &bn;
	args.structureInputSize = sizeof(bn);
	args.structureOutput = &bn;
	args.structureOutputSize = sizeof(bn);
	ret = externalMethod(kLavBufNew, &args, NULL, NULL, NULL);
	IOLog("LavenderGPU: selftest buffer: ret %x handle %u address %llx size %llu\n", ret, bn.handle, bn.address, bn.size);
	if (ret != kIOReturnSuccess || bn.handle == 0) {
		return false;
	}
	words = (volatile uint32_t *)fMemory[bn.handle - 1]->getBytesNoCopy();

	stream[0] = lav_pkt7(61, 3);                    /* CP_MEM_WRITE: address, then the words */
	stream[1] = (uint32_t)bn.address;
	stream[2] = (uint32_t)(bn.address >> 32);
	stream[3] = 0x5eed1234u;
	stream[4] = lav_pkt7(61, 3);
	stream[5] = (uint32_t)(bn.address + 4);
	stream[6] = (uint32_t)((bn.address + 4) >> 32);
	stream[7] = 0x600dcafeu;
	bzero(&args, sizeof(args));
	bzero(&out, sizeof(out));
	args.structureInput = stream;
	args.structureInputSize = sizeof(stream);
	args.structureOutput = &out;
	args.structureOutputSize = sizeof(out);
	ret = externalMethod(kLavSubmit, &args, NULL, NULL, NULL);
	IOLog("LavenderGPU: selftest submit: ret %x fence %u status %u, buffer words %08x %08x (want 5eed1234 600dcafe)\n",
	    ret, out.fence, out.status, words[0], words[1]);
	ok = ret == kIOReturnSuccess && words[0] == 0x5eed1234u && words[1] == 0x600dcafeu;

	bzero(&bf, sizeof(bf));
	bf.handle = bn.handle;
	bzero(&args, sizeof(args));
	args.structureInput = &bf;
	args.structureInputSize = sizeof(bf);
	ret = externalMethod(kLavBufFree, &args, NULL, NULL, NULL);
	IOLog("LavenderGPU: selftest free: ret %x; %s\n", ret, ok ? "PASSED" : "FAILED");
	return ok;
}

void
LavenderGPUSelfTest(void)
{
	LavenderGPUUserClient * client = OSTypeAlloc(LavenderGPUUserClient);

	if (client == NULL || !client->init() || !client->initWithTask(kernel_task, NULL, 0, NULL)) {
		IOLog("LavenderGPU: selftest: no client\n");
		OSSafeReleaseNULL(client);
		return;
	}
	client->selfTest();
	client->clientClose();
	client->release();
}
