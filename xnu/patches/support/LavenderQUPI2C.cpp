#include <pexpert/arm64/board_config.h>
#if defined(ARM64_BOARD_CONFIG_LAVENDER)
#include "LavenderQUPI2C.h"
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IODeviceMemory.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOCatalogue.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOLocks.h>
#include <libkern/c++/OSSharedPtr.h>
#include <libkern/c++/OSString.h>
#include <libkern/c++/OSArray.h>
#include <libkern/c++/OSDictionary.h>
#include <libkern/c++/OSBoolean.h>
#include <kern/clock.h>
#include <kern/thread.h>
#define QUP_CONFIG 0x000
#define QUP_STATE 0x004
#define QUP_IO_MODES 0x008
#define QUP_SW_RESET 0x00c
#define QUP_OPERATIONAL 0x018
#define QUP_ERROR_FLAGS 0x01c
#define QUP_ERROR_FLAGS_EN 0x020
#define QUP_OPERATIONAL_MASK 0x028
#define QUP_MX_OUTPUT_COUNT 0x100
#define QUP_OUT_FIFO_BASE 0x110
#define QUP_MX_WRITE_COUNT 0x150
#define QUP_MX_INPUT_COUNT 0x200
#define QUP_MX_READ_COUNT 0x208
#define QUP_IN_FIFO_BASE 0x218
#define QUP_I2C_MASTER_CLK_CTL 0x400
#define QUP_I2C_STATUS 0x404
#define QUP_I2C_MASTER_CONFIG 0x408
#define QUP_STATE_MASK 0x3U
#define QUP_STATE_VALID (1U<<2)
#define QUP_I2C_MAST_GEN (1U<<4)
#define QUP_STATE_RESET 0U
#define QUP_STATE_RUN 1U
#define QUP_STATE_PAUSE 3U
#define QUP_N_VAL 0x7U
#define QUP_MINI_CORE_I2C_VAL 0x200U
#define QUP_NO_INPUT (1U<<7)
#define QUP_PACK_EN (1U<<15)
#define QUP_UNPACK_EN (1U<<14)
#define QUP_OUTPUT_SERVICE_FLAG (1U<<8)
#define QUP_INPUT_SERVICE_FLAG (1U<<9)
#define QUP_MAX_OUTPUT_DONE_FLAG (1U<<10)
#define QUP_MAX_INPUT_DONE_FLAG (1U<<11)
#define QUP_SERVICE_FLAGS (QUP_OUTPUT_SERVICE_FLAG|QUP_INPUT_SERVICE_FLAG)
#define QUP_I2C_ERROR_MASK 0x0380003cU
#define QUP_I2C_NACK (1U<<3)
#define QUP_I2C_ARB_LOST (1U<<4)
#define QUP_I2C_BUS_ERROR (1U<<2)
#define QUP_ERROR_FLAGS_MASK 0x3cU
#define QUP_EN_VERSION_TWO_TAG 1U
#define GCC_PHYS_BASE 0x00100000ULL
#define GCC_PHYS_LENGTH 0x00094000ULL
#define GCC_BLSP1_AHB_VOTE 0x52004
#define GCC_BLSP1_AHB_HALT 0x17004
#define GCC_QUP1_I2C_CMD_RCGR 0x19020
#define GCC_QUP1_I2C_CFG_RCGR 0x19024
#define GCC_QUP1_I2C_BRANCH 0x19008
#define GCC_BRANCH_HALT (1U<<31)
#define GCC_QUP1_AHB_VOTE_BIT (1U<<17)
#define GCC_QUP1_I2C_BRANCH_BIT 1U
#define GCC_RCG_UPDATE 1U
static inline uint32_t qread(volatile uint8_t *b,uint32_t o){return OSReadLittleInt32((const volatile void *)b,o);}
static inline void qwrite(volatile uint8_t *b,uint32_t o,uint32_t v){OSWriteLittleInt32((volatile void *)b,o,v);OSSynchronizeIO();}
static bool qwait(volatile uint8_t *b,uint32_t o,uint32_t m,uint32_t e,uint32_t n){while(n--){if((qread(b,o)&m)==e)return true;IOSleep(1);}return false;}
class LavenderQUPI2C : public IOService {
    OSDeclareDefaultStructors(LavenderQUPI2C);
public:
    IOService *probe(IOService *provider,SInt32 *score) APPLE_KEXT_OVERRIDE;
    bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
    IOReturn callPlatformFunction(const OSSymbol *name,bool wait,void *p1,void *p2,void *p3,void *p4) APPLE_KEXT_OVERRIDE;
    IOReturn transfer(LavenderI2CMessage *messages,uint32_t count,uint32_t timeoutMilliseconds=500);
private:
    static void irqAction(OSObject *owner,IOInterruptEventSource *sender,int count);
    void handleIRQ(void);
    bool enableClocks(void); void disableClocks(void);
    bool initController(void); bool setState(uint32_t state);
    bool fifoSizes(uint32_t *in,uint32_t *out); void releaseResources(void);
    IOMemoryMap *fQUPMap=nullptr,*fGCCMap=nullptr;
    volatile uint8_t *fQUP=nullptr,*fGCC=nullptr;
    IOWorkLoop *fWorkLoop=nullptr; IOInterruptEventSource *fIRQ=nullptr; IOLock *fLock=nullptr;
    volatile bool fActive=false; bool fDone=false,fLastRead=false,fClocks=false;
    IOReturn fResult=kIOReturnSuccess; uint32_t fEvent=0;
};
OSDefineMetaClassAndStructors(LavenderQUPI2C,IOService);
IOService *LavenderQUPI2C::probe(IOService *p,SInt32 *score){
    if(!p||p->getResources()!=kIOReturnSuccess||!p->getDeviceMemoryCount())return nullptr;
    IODeviceMemory *m=p->getDeviceMemoryWithIndex(0);
    if(!m||m->getPhysicalAddress()!=LAVENDER_QUP_I2C_BASE||m->getLength()<LAVENDER_QUP_I2C_LENGTH)return nullptr;
    if(score)*score=1000; return this;
}
bool LavenderQUPI2C::enableClocks(void){
    OSPtr<IODeviceMemory> mem=IODeviceMemory::withRange(GCC_PHYS_BASE,GCC_PHYS_LENGTH);
    if(!mem)return false;
    OSPtr<IOMemoryMap> map=mem->map(kIOMapInhibitCache);
    if(!map||map->getLength()<GCC_PHYS_LENGTH)return false;
    fGCCMap=map.get();fGCCMap->retain();fGCC=(volatile uint8_t *)fGCCMap->getVirtualAddress();
    if(!fGCC)return false;
    uint32_t v=qread(fGCC,GCC_BLSP1_AHB_VOTE);
    qwrite(fGCC,GCC_BLSP1_AHB_VOTE,v|GCC_QUP1_AHB_VOTE_BIT);fClocks=true;
    if(!qwait(fGCC,GCC_BLSP1_AHB_HALT,GCC_BRANCH_HALT,0,100))return false;
    v=qread(fGCC,GCC_QUP1_I2C_CFG_RCGR);qwrite(fGCC,GCC_QUP1_I2C_CFG_RCGR,(v&~0x71fU)|1U);
    v=qread(fGCC,GCC_QUP1_I2C_CMD_RCGR);qwrite(fGCC,GCC_QUP1_I2C_CMD_RCGR,v|GCC_RCG_UPDATE);
    if(!qwait(fGCC,GCC_QUP1_I2C_CMD_RCGR,GCC_RCG_UPDATE,0,100))return false;
    v=qread(fGCC,GCC_QUP1_I2C_BRANCH);qwrite(fGCC,GCC_QUP1_I2C_BRANCH,v|GCC_QUP1_I2C_BRANCH_BIT);
    return qwait(fGCC,GCC_QUP1_I2C_BRANCH,GCC_BRANCH_HALT,0,100);
}
void LavenderQUPI2C::disableClocks(void){
    if(!fGCC||!fClocks)return;
    uint32_t v=qread(fGCC,GCC_QUP1_I2C_BRANCH);qwrite(fGCC,GCC_QUP1_I2C_BRANCH,v&~GCC_QUP1_I2C_BRANCH_BIT);
    v=qread(fGCC,GCC_BLSP1_AHB_VOTE);qwrite(fGCC,GCC_BLSP1_AHB_VOTE,v&~GCC_QUP1_AHB_VOTE_BIT);fClocks=false;
}
bool LavenderQUPI2C::setState(uint32_t state){
    if(!fQUP)return false;qwrite(fQUP,QUP_STATE,state);
    for(uint32_t i=0;i<100;i++){uint32_t v=qread(fQUP,QUP_STATE);if((v&QUP_STATE_VALID)&&((v&QUP_STATE_MASK)==state))return true;IOSleep(1);}return false;
}
bool LavenderQUPI2C::initController(void){
    qwrite(fQUP,QUP_SW_RESET,1);IODelay(5);
    if(!qwait(fQUP,QUP_STATE,QUP_STATE_VALID,QUP_STATE_VALID,100)||!setState(QUP_STATE_RESET))return false;
    qwrite(fQUP,QUP_CONFIG,QUP_N_VAL|QUP_MINI_CORE_I2C_VAL);
    qwrite(fQUP,QUP_ERROR_FLAGS_EN,QUP_ERROR_FLAGS_MASK);
    qwrite(fQUP,QUP_OPERATIONAL_MASK,QUP_SERVICE_FLAGS);
    qwrite(fQUP,QUP_I2C_MASTER_CONFIG,QUP_EN_VERSION_TWO_TAG);
    return (qread(fQUP,QUP_STATE)&QUP_I2C_MAST_GEN)!=0;
}
bool LavenderQUPI2C::fifoSizes(uint32_t *in,uint32_t *out){
    static const uint8_t blk[4]={16,16,32,0},sz[4]={2,4,8,16};
    if(!in||!out||!fQUP)return false;uint32_t m=qread(fQUP,QUP_IO_MODES);
    *out=blk[m&3U]*sz[(m>>2)&3U];*in=blk[(m>>5)&3U]*sz[(m>>7)&3U];return *in&&*out;
}
bool LavenderQUPI2C::start(IOService *p){
    if(!super::start(p))return false;
    fQUPMap=p->mapDeviceMemoryWithIndex(0,kIOMapInhibitCache);
    if(!fQUPMap||fQUPMap->getPhysicalAddress()!=LAVENDER_QUP_I2C_BASE||fQUPMap->getLength()<LAVENDER_QUP_I2C_LENGTH){releaseResources();super::stop(p);return false;}
    fQUP=(volatile uint8_t *)fQUPMap->getVirtualAddress();
    if(!fQUP||!enableClocks()||!initController()){releaseResources();super::stop(p);return false;}
    fLock=IOLockAlloc();fWorkLoop=getWorkLoop();
    if(!fLock||!fWorkLoop){releaseResources();super::stop(p);return false;}fWorkLoop->retain();
    OSPtr<IOInterruptEventSource> src=IOInterruptEventSource::interruptEventSource(this,&LavenderQUPI2C::irqAction,p,0);
    if(!src){releaseResources();super::stop(p);return false;}
    fIRQ=src.get();fIRQ->retain();
    if(fWorkLoop->addEventSource(fIRQ)!=kIOReturnSuccess){releaseResources();super::stop(p);return false;}
    fIRQ->enable();setProperty("LavenderQUPI2CReady",kOSBooleanTrue);registerService();
    IOService::publishResource(LAVENDER_I2C_TRANSFER_FUNCTION,this);
    IOLog("LAVENDER QUP: BLSP1 QUP1 online at 0x%llx; tagged FIFO transport registered\n",(unsigned long long)LAVENDER_QUP_I2C_BASE);
    return true;
}
void LavenderQUPI2C::releaseResources(void){
    if(fIRQ){fIRQ->disable();if(fWorkLoop)fWorkLoop->removeEventSource(fIRQ);fIRQ->release();fIRQ=nullptr;}
    if(fWorkLoop){fWorkLoop->release();fWorkLoop=nullptr;}
    if(fLock){IOLockFree(fLock);fLock=nullptr;}
    disableClocks();
    if(fGCCMap){fGCCMap->release();fGCCMap=nullptr;fGCC=nullptr;}
    if(fQUPMap){fQUPMap->release();fQUPMap=nullptr;fQUP=nullptr;}
}
void LavenderQUPI2C::stop(IOService *p){if(fQUP)setState(QUP_STATE_RESET);releaseResources();super::stop(p);}
void LavenderQUPI2C::irqAction(OSObject *o,IOInterruptEventSource *,int){LavenderQUPI2C *s=OSDynamicCast(LavenderQUPI2C,o);if(s)s->handleIRQ();}
void LavenderQUPI2C::handleIRQ(void){
    if(!fQUP)return;uint32_t st=qread(fQUP,QUP_I2C_STATUS),er=qread(fQUP,QUP_ERROR_FLAGS),op=qread(fQUP,QUP_OPERATIONAL);
    IOReturn r=kIOReturnSuccess;bool done=false;
    if(st&QUP_I2C_ERROR_MASK){r=(st&QUP_I2C_NACK)?kIOReturnNotResponding:(st&QUP_I2C_ARB_LOST)?kIOReturnBusy:kIOReturnIOError;done=true;}
    if(er&QUP_ERROR_FLAGS_MASK){r=kIOReturnIOError;done=true;}
    if(st&QUP_I2C_ERROR_MASK)qwrite(fQUP,QUP_I2C_STATUS,st&QUP_I2C_ERROR_MASK);
    if(er&QUP_ERROR_FLAGS_MASK)qwrite(fQUP,QUP_ERROR_FLAGS,er&QUP_ERROR_FLAGS_MASK);
    if(op&QUP_SERVICE_FLAGS)qwrite(fQUP,QUP_OPERATIONAL,op&QUP_SERVICE_FLAGS);
    if(!done&&fActive)done=fLastRead?((op&QUP_MAX_INPUT_DONE_FLAG)!=0):((op&(QUP_OUTPUT_SERVICE_FLAG|QUP_MAX_OUTPUT_DONE_FLAG))!=0);
    if(done&&fLock){IOLockLock(fLock);if(fActive&&!fDone){fResult=r;fDone=true;if(r!=kIOReturnSuccess)qwrite(fQUP,QUP_STATE,QUP_STATE_RESET);IOLockWakeup(fLock,&fEvent,false);}IOLockUnlock(fLock);}
}
IOReturn LavenderQUPI2C::callPlatformFunction(const OSSymbol *name,bool wait,void *p1,void *p2,void *p3,void *p4){
    if(!name||!name->isEqualTo(LAVENDER_I2C_TRANSFER_FUNCTION))return super::callPlatformFunction(name,wait,p1,p2,p3,p4);
    LavenderI2CTransfer *r=(LavenderI2CTransfer *)p1;if(!r)return kIOReturnBadArgument;
    return transfer(r->messages,r->messageCount,r->timeoutMilliseconds?r->timeoutMilliseconds:500);
}
IOReturn LavenderQUPI2C::transfer(LavenderI2CMessage *m,uint32_t n,uint32_t timeout){
    LavenderQUPTransferSize z={};uint32_t inCap,outCap,written=0,inPos=0,ioModes;uint8_t *out=nullptr,*in=nullptr;
    IOReturn result=kIOReturnSuccess;AbsoluteTime deadline;int wr;
    if(!fQUP||!fLock||!fClocks)return kIOReturnNotReady;
    if(!lavender_qup_measure(m,n,&z))return kIOReturnBadArgument;
    if(!fifoSizes(&inCap,&outCap)||z.outputBytes>outCap||z.inputBytes>inCap)return kIOReturnNoResources;
    out=(uint8_t *)IOMalloc(z.outputBytes+4);if(z.inputBytes)in=(uint8_t *)IOMalloc(z.inputBytes+4);
    if(!out||(z.inputBytes&&!in)){result=kIOReturnNoMemory;goto done;}
    if(!lavender_qup_build_output(m,n,out,z.outputBytes+4,&written)||written!=z.outputBytes){result=kIOReturnBadArgument;goto done;}
    IOLockLock(fLock);if(fActive){IOLockUnlock(fLock);result=kIOReturnBusy;goto done;}
    fActive=true;fDone=false;fLastRead=z.lastIsRead;fResult=kIOReturnSuccess;
    if(!setState(QUP_STATE_RESET)){result=kIOReturnTimeout;goto unlock;}
    qwrite(fQUP,QUP_MX_INPUT_COUNT,0);qwrite(fQUP,QUP_MX_OUTPUT_COUNT,0);
    qwrite(fQUP,QUP_MX_READ_COUNT,z.inputBytes);qwrite(fQUP,QUP_MX_WRITE_COUNT,z.outputBytes);
    qwrite(fQUP,QUP_CONFIG,QUP_N_VAL|QUP_MINI_CORE_I2C_VAL|(z.inputBytes?0:QUP_NO_INPUT));
    ioModes=qread(fQUP,QUP_IO_MODES);ioModes&=~((3U<<10)|(3U<<12)|QUP_PACK_EN|QUP_UNPACK_EN|(1U<<16));
    qwrite(fQUP,QUP_IO_MODES,ioModes|QUP_PACK_EN|QUP_UNPACK_EN);
    qwrite(fQUP,QUP_I2C_STATUS,QUP_I2C_ERROR_MASK);qwrite(fQUP,QUP_ERROR_FLAGS,QUP_ERROR_FLAGS_MASK);
    qwrite(fQUP,QUP_OPERATIONAL_MASK,0);
    if(!setState(QUP_STATE_RUN)){result=kIOReturnTimeout;goto unlock;}
    qwrite(fQUP,QUP_I2C_MASTER_CLK_CTL,(14U<<16)|28U);
    if(!setState(QUP_STATE_PAUSE)){result=kIOReturnTimeout;goto unlock;}
    for(uint32_t i=0;i<written;i+=4){uint32_t w=out[i];if(i+1<written)w|=(uint32_t)out[i+1]<<8;if(i+2<written)w|=(uint32_t)out[i+2]<<16;if(i+3<written)w|=(uint32_t)out[i+3]<<24;qwrite(fQUP,QUP_OUT_FIFO_BASE,w);}
    if(!setState(QUP_STATE_RUN)){result=kIOReturnTimeout;goto unlock;}
    clock_interval_to_deadline(timeout,kMillisecondScale,&deadline);
    wr=fDone?THREAD_AWAKENED:IOLockSleepDeadline(fLock,&fEvent,deadline,THREAD_UNINT);
    if(!fDone||wr==THREAD_TIMED_OUT){setState(QUP_STATE_RESET);result=kIOReturnTimeout;goto unlock;}
    result=fResult;if(result!=kIOReturnSuccess)goto unlock;
    while(inPos<z.inputBytes){uint32_t w=qread(fQUP,QUP_IN_FIFO_BASE);for(uint32_t b=0;b<4&&inPos<z.inputBytes;b++)in[inPos++]=(uint8_t)(w>>(b*8));}
    if(z.inputBytes&&!lavender_qup_consume_input(m,n,in,z.inputBytes))result=kIOReturnIOError;
    setState(QUP_STATE_RESET);
unlock:
    fActive=false;IOLockUnlock(fLock);
done:
    if(out)IOFree(out,z.outputBytes+4);if(in)IOFree(in,z.inputBytes+4);return result;
}
extern "C" void lavender_register_qup_i2c_driver(void){
    OSPtr<OSDictionary> p=OSDictionary::withCapacity(4);OSPtr<OSArray> a=OSArray::withCapacity(1);
    OSPtr<OSString> c=OSString::withCStringNoCopy("LavenderQUPI2C");
    OSPtr<OSString> pc=OSString::withCStringNoCopy("IOPlatformDevice");
    OSPtr<OSString> n=OSString::withCStringNoCopy("qcom,i2c-msm-v2");
    OSPtr<OSString> mc=OSString::withCStringNoCopy("LavenderQUPI2C");
    if(!p||!a||!c||!pc||!n||!mc){IOLog("LAVENDER QUP: personality allocation failed\n");return;}
    p->setObject("IOClass",c.get());p->setObject("IOProviderClass",pc.get());p->setObject("IONameMatch",n.get());p->setObject("IOMatchCategory",mc.get());
    a->setObject(p.get());
    if(!gIOCatalogue||!gIOCatalogue->addDrivers(a.get(),true))IOLog("LAVENDER QUP: personality registration failed\n");
    else IOLog("LAVENDER QUP: personality registered\n");
}
#endif
