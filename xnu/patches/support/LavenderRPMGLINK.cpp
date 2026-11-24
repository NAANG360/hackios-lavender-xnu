#include <pexpert/arm64/board_config.h>
#if defined(ARM64_BOARD_CONFIG_LAVENDER)
#include "LavenderRPMGLINK.h"
#include <IOKit/IOService.h>
#include <pexpert/pexpert.h>
#include <IOKit/IODeviceMemory.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOLocks.h>
#include <IOKit/IOCatalogue.h>
#include <libkern/OSByteOrder.h>
#include <libkern/c++/OSString.h>
#include <libkern/c++/OSArray.h>
#include <libkern/c++/OSDictionary.h>

static inline uint32_t rpmRead32(volatile uint8_t *base,uint32_t offset) {
    return OSReadLittleInt32((const volatile void *)base,offset);
}
static inline void rpmWrite32(volatile uint8_t *base,uint32_t offset,uint32_t value) {
    OSWriteLittleInt32((volatile void *)base,offset,value);
    OSSynchronizeIO();
}

class LavenderRPMGLINK : public IOService {
    OSDeclareDefaultStructors(LavenderRPMGLINK);
public:
    using super=IOService;
    IOService *probe(IOService *provider,SInt32 *score) APPLE_KEXT_OVERRIDE;
    bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
    bool requestVddDigLowSVS(void);
private:
    bool mapResources(void);
    void releaseResources(void);
    bool sendVersion(void);
    bool sendCommand(uint16_t command,uint16_t p1,uint32_t p2);
    bool sendOpenRequest(void);
    bool sendRPMData(const uint8_t *data,uint32_t length);
    bool writeTX(const uint8_t *data,uint32_t length);
    bool pumpRX(void);
    bool readRX(uint32_t position,uint8_t *out,uint32_t length);
    bool advanceRX(uint32_t length);
    bool waitForChannel(void);
    void handleRPMData(const uint8_t *data,uint32_t length);
    uint32_t nextMessageId(void);

    IOMemoryMap *fRAMMap=nullptr,*fAPCSMap=nullptr;
    volatile uint8_t *fRAM=nullptr,*fAPCS=nullptr;
    IOLock *fLock=nullptr;
    LavenderRPMRings fRings={};
    bool fStarted=false,fVersionAck=false,fOpenPending=false,fChannelReady=false;
    bool fVoteGranted=false,fAckSeen=false,fRXAssemblyActive=false;
    uint16_t fLocalCID=1,fRemoteCID=0;
    uint32_t fNextMessageId=1,fLastAckId=0;
    int32_t fLastAckStatus=-1;
    uint32_t fRXAssemblyExpected=0,fRXAssemblyOffset=0;
    uint8_t fRXAssembly[LAVENDER_GLINK_MAX_PACKET]={};
};
OSDefineMetaClassAndStructors(LavenderRPMGLINK,IOService);

static LavenderRPMGLINK *gLavenderRPMGLINK=nullptr;

IOService *LavenderRPMGLINK::probe(IOService *provider,SInt32 *score) {
    int enabled=1;
    if(PE_parse_boot_argn("lavenderrpm",&enabled,sizeof(enabled))&&!enabled)return nullptr;
    if(!provider)return nullptr;
    if(score)*score=1000;
    return this;
}

bool LavenderRPMGLINK::mapResources(void) {
    IODeviceMemory *ram=IODeviceMemory::withRange(LAVENDER_RPM_MSG_RAM_BASE,
                                                   LAVENDER_RPM_MSG_RAM_SIZE);
    IODeviceMemory *apcs=IODeviceMemory::withRange(LAVENDER_RPM_APCS_BASE,0x1000U);
    if(!ram||!apcs){if(ram)ram->release();if(apcs)apcs->release();return false;}
    fRAMMap=ram->map(kIOMapInhibitCache);
    fAPCSMap=apcs->map(kIOMapInhibitCache);
    ram->release();apcs->release();
    if(!fRAMMap||!fAPCSMap||fRAMMap->getLength()<LAVENDER_RPM_MSG_RAM_SIZE||
       fAPCSMap->getLength()<0x1000U)return false;
    fRAM=(volatile uint8_t *)fRAMMap->getVirtualAddress();
    fAPCS=(volatile uint8_t *)fAPCSMap->getVirtualAddress();
    return fRAM&&fAPCS;
}

void LavenderRPMGLINK::releaseResources(void) {
    if(fAPCSMap){fAPCSMap->release();fAPCSMap=nullptr;}
    if(fRAMMap){fRAMMap->release();fRAMMap=nullptr;}
    fAPCS=nullptr;fRAM=nullptr;
    if(fLock){IOLockFree(fLock);fLock=nullptr;}
}

bool LavenderRPMGLINK::writeTX(const uint8_t *data,uint32_t length) {
    if(!fRAM||!data||!length||(length&7U))return false;
    const LavenderRPMRing &ring=fRings.tx;
    uint32_t head=rpmRead32(fRAM,ring.headOffset);
    uint32_t tail=rpmRead32(fRAM,ring.tailOffset);
    uint32_t freeBytes=0;
    if(!lavender_rpm_ring_free(head,tail,ring.length,&freeBytes)||length>freeBytes)
        return false;
    for(uint32_t i=0;i<length;i+=4U) {
        uint32_t word=(uint32_t)data[i]|
                      ((uint32_t)data[i+1U]<<8)|
                      ((uint32_t)data[i+2U]<<16)|
                      ((uint32_t)data[i+3U]<<24);
        uint32_t position=(head+i)%ring.length;
        if(position&3U)return false;
        rpmWrite32(fRAM,ring.fifoOffset+position,word);
    }
    uint32_t next=(head+length)%ring.length;
    rpmWrite32(fRAM,ring.headOffset,next);
    /* qcom-apcs-ipc-mailbox: mailbox channel zero rings bit zero at +0x8. */
    rpmWrite32(fAPCS,LAVENDER_RPM_APCS_DOORBELL,LAVENDER_RPM_APCS_DOORBELL_BIT);
    return true;
}

bool LavenderRPMGLINK::sendCommand(uint16_t command,uint16_t p1,uint32_t p2) {
    uint8_t packet[8]={};
    return lavender_rpm_encode_glink_header(packet,sizeof(packet),command,p1,p2)&&
           writeTX(packet,sizeof(packet));
}

bool LavenderRPMGLINK::sendVersion(void) {
    return sendCommand(LAVENDER_GLINK_CMD_VERSION,LAVENDER_GLINK_VERSION,
                       LAVENDER_GLINK_FEATURES);
}

bool LavenderRPMGLINK::sendOpenRequest(void) {
    uint8_t packet[24]={};
    if(!lavender_rpm_encode_glink_open(packet,sizeof(packet),fLocalCID))return false;
    return writeTX(packet,sizeof(packet));
}

bool LavenderRPMGLINK::sendRPMData(const uint8_t *data,uint32_t length) {
    uint8_t packet[16U+LAVENDER_GLINK_MAX_PACKET+8U]={};
    uint32_t encoded=0;
    if(!lavender_rpm_encode_glink_tx_data(packet,sizeof(packet),fLocalCID,
                                           data,length,&encoded))return false;
    return writeTX(packet,encoded);
}

bool LavenderRPMGLINK::readRX(uint32_t position,uint8_t *out,uint32_t length) {
    if(!fRAM||!out||position>=fRings.rx.length||length>fRings.rx.length)return false;
    for(uint32_t i=0;i<length;i++)
        out[i]=fRAM[fRings.rx.fifoOffset+((position+i)%fRings.rx.length)];
    return true;
}

bool LavenderRPMGLINK::advanceRX(uint32_t length) {
    uint32_t tail=rpmRead32(fRAM,fRings.rx.tailOffset);
    if(tail>=fRings.rx.length||length>fRings.rx.length)return false;
    rpmWrite32(fRAM,fRings.rx.tailOffset,(tail+length)%fRings.rx.length);
    return true;
}

void LavenderRPMGLINK::handleRPMData(const uint8_t *data,uint32_t length) {
    uint32_t messageId=0;int32_t status=-1;
    if(!lavender_rpm_decode_v0_ack(data,length,&messageId,&status))return;
    fLastAckId=messageId;fLastAckStatus=status;fAckSeen=true;
}

bool LavenderRPMGLINK::pumpRX(void) {
    if(!fRAM)return false;
    for(uint32_t packetCount=0;packetCount<64U;packetCount++) {
        uint32_t head=rpmRead32(fRAM,fRings.rx.headOffset);
        uint32_t tail=rpmRead32(fRAM,fRings.rx.tailOffset);
        uint32_t available=0;
        if(!lavender_rpm_ring_available(head,tail,fRings.rx.length,&available))return false;
        if(available<8U)return true;
        uint8_t header[16]={};
        if(!readRX(tail,header,8U))return false;
        uint16_t command=lavender_rpm_le16(header);
        uint16_t p1=lavender_rpm_le16(header+2);
        uint32_t p2=lavender_rpm_le32(header+4);
        switch(command) {
        case LAVENDER_GLINK_CMD_VERSION:
            if(p1==LAVENDER_GLINK_VERSION) {
                if(!sendCommand(LAVENDER_GLINK_CMD_VERSION_ACK,
                                LAVENDER_GLINK_VERSION,LAVENDER_GLINK_FEATURES))return false;
            }
            if(!advanceRX(8U))return false;
            break;
        case LAVENDER_GLINK_CMD_VERSION_ACK:
            fVersionAck=(p1==LAVENDER_GLINK_VERSION&&p2==LAVENDER_GLINK_FEATURES);
            if(!fVersionAck&&p1!=0U&&!sendVersion())return false;
            if(!advanceRX(8U))return false;
            break;
        case LAVENDER_GLINK_CMD_OPEN: {
            if(p2==0U||p2>64U||available<lavender_rpm_align8(8U+p2))return true;
            uint8_t name[64]={};
            if(!readRX((tail+8U)%fRings.rx.length,name,p2))return false;
            static const char expected[LAVENDER_RPM_CHANNEL_LENGTH]="rpm_requests";
            bool matches=p2==LAVENDER_RPM_CHANNEL_LENGTH&&
                         __builtin_memcmp(name,expected,LAVENDER_RPM_CHANNEL_LENGTH)==0;
            if(matches) {
                if(p1==0U)return false;
                fRemoteCID=p1;
                if(!sendCommand(LAVENDER_GLINK_CMD_OPEN_ACK,fRemoteCID,0U))return false;
                fOpenPending=true;
                if(!sendOpenRequest())return false;
                IOLog("LAVENDER RPM: remote opened rpm_requests (remote CID=%u, local CID=%u)\n",
                      fRemoteCID,fLocalCID);
            }
            if(!advanceRX(lavender_rpm_align8(8U+p2)))return false;
            break;
        }
        case LAVENDER_GLINK_CMD_OPEN_ACK:
            if(p1==fLocalCID&&fOpenPending) {
                fChannelReady=true;
                IOLog("LAVENDER RPM: rpm_requests GLINK/RPMSG channel open\n");
            }
            if(!advanceRX(8U))return false;
            break;
        case LAVENDER_GLINK_CMD_CLOSE:
            if(p1==fRemoteCID) {
                sendCommand(LAVENDER_GLINK_CMD_CLOSE_ACK,fRemoteCID,0U);
                fChannelReady=false;fOpenPending=false;fRemoteCID=0;
            }
            if(!advanceRX(8U))return false;
            break;
        case LAVENDER_GLINK_CMD_CLOSE_ACK:
            fChannelReady=false;fOpenPending=false;
            if(!advanceRX(8U))return false;
            break;
        case LAVENDER_GLINK_CMD_READ_NOTIF:
            if(!advanceRX(8U))return false;
            break;
        case LAVENDER_GLINK_CMD_TX_DATA:
        case LAVENDER_GLINK_CMD_TX_DATA_CONT: {
            if(available<16U)return true;
            if(!readRX(tail,header,16U))return false;
            uint32_t chunk=lavender_rpm_le32(header+8);
            uint32_t left=lavender_rpm_le32(header+12);
            if(chunk>LAVENDER_GLINK_MAX_PACKET||left>LAVENDER_GLINK_MAX_PACKET||
               chunk>LAVENDER_GLINK_MAX_PACKET-left)return false;
            uint32_t total=chunk+left;
            if(available<16U+chunk)return true;
            if(p1!=fRemoteCID) {
                if(!advanceRX(lavender_rpm_align8(16U+chunk)))return false;
                break;
            }
            if(command==LAVENDER_GLINK_CMD_TX_DATA) {
                if(fRXAssemblyActive)return false;
                fRXAssemblyExpected=total;fRXAssemblyOffset=0;fRXAssemblyActive=true;
            } else if(!fRXAssemblyActive) {
                return false;
            }
            if(fRXAssemblyOffset+chunk+left!=fRXAssemblyExpected||
               fRXAssemblyOffset+chunk>sizeof(fRXAssembly))return false;
            if(!readRX((tail+16U)%fRings.rx.length,
                       fRXAssembly+fRXAssemblyOffset,chunk))return false;
            fRXAssemblyOffset+=chunk;
            if(!advanceRX(lavender_rpm_align8(16U+chunk)))return false;
            if(left==0U) {
                if(fRXAssemblyOffset!=fRXAssemblyExpected)return false;
                handleRPMData(fRXAssembly,fRXAssemblyOffset);
                fRXAssemblyActive=false;fRXAssemblyExpected=0;fRXAssemblyOffset=0;
            }
            break;
        }
        default:
            IOLog("LAVENDER RPM: unsupported GLINK command %u; stopping RX pump\n",command);
            return false;
        }
    }
    return true;
}

bool LavenderRPMGLINK::waitForChannel(void) {
    for(uint32_t elapsed=0;elapsed<LAVENDER_GLINK_POLL_TIMEOUT_MS;elapsed++) {
        if(!pumpRX())return false;
        if(fVersionAck&&fChannelReady)return true;
        IOSleep(1);
    }
    IOLog("LAVENDER RPM: rpm_requests open timeout (version=%u open=%u)\n",
          fVersionAck?1U:0U,fChannelReady?1U:0U);
    return false;
}

uint32_t LavenderRPMGLINK::nextMessageId(void) {
    do { ++fNextMessageId; } while(fNextMessageId<2U);
    return fNextMessageId;
}

bool LavenderRPMGLINK::requestVddDigLowSVS(void) {
    if(!fStarted||!fLock)return false;
    IOLockLock(fLock);
    if(fVoteGranted){IOLockUnlock(fLock);return true;}
    if(!waitForChannel()){IOLockUnlock(fLock);return false;}
    uint8_t request[LAVENDER_RPM_V0_REQUEST_SIZE]={};
    uint32_t requestLength=0;
    uint32_t messageId=nextMessageId();
    fAckSeen=false;
    if(!lavender_rpm_encode_v0_vdd_dig_vote(request,sizeof(request),messageId,
                                             LAVENDER_RPM_LEVEL_LOW_SVS,&requestLength)||
       !sendRPMData(request,requestLength)) {
        IOLog("LAVENDER RPM: failed to submit vdd_dig LOW_SVS request\n");
        IOLockUnlock(fLock);return false;
    }
    for(uint32_t elapsed=0;elapsed<LAVENDER_RPM_ACK_TIMEOUT_MS;elapsed++) {
        if(!pumpRX())break;
        if(fAckSeen&&fLastAckId==messageId) {
            if(fLastAckStatus==0) {
                fVoteGranted=true;
                IOLog("LAVENDER RPM: vdd_dig LOW_SVS acknowledged (msg=%u)\n",messageId);
                IOLockUnlock(fLock);return true;
            }
            IOLog("LAVENDER RPM: vdd_dig request NACK (msg=%u status=%d)\n",
                  messageId,fLastAckStatus);
            IOLockUnlock(fLock);return false;
        }
        IOSleep(1);
    }
    IOLog("LAVENDER RPM: vdd_dig LOW_SVS ACK timeout (msg=%u)\n",messageId);
    IOLockUnlock(fLock);return false;
}

bool LavenderRPMGLINK::start(IOService *provider) {
    if(!super::start(provider))return false;
    fLock=IOLockAlloc();
    if(!fLock||!mapResources()){releaseResources();super::stop(provider);return false;}
    uint8_t *snapshot=(uint8_t *)IOMalloc(LAVENDER_RPM_MSG_RAM_SIZE);
    if(!snapshot){releaseResources();super::stop(provider);return false;}
    for(uint32_t i=0;i<LAVENDER_RPM_MSG_RAM_SIZE;i++)snapshot[i]=fRAM[i];
    bool tocOK=lavender_rpm_parse_toc(snapshot,LAVENDER_RPM_MSG_RAM_SIZE,&fRings);
    IOFree(snapshot,LAVENDER_RPM_MSG_RAM_SIZE);
    if(!tocOK) {
        IOLog("LAVENDER RPM: invalid RPM message-RAM TOC at 0x%llx\n",
              (unsigned long long)LAVENDER_RPM_MSG_RAM_BASE);
        releaseResources();super::stop(provider);return false;
    }
    /* Match qcom_glink_rpm_probe: initialize AP-owned TX head and RX tail only. */
    rpmWrite32(fRAM,fRings.tx.headOffset,0U);
    rpmWrite32(fRAM,fRings.rx.tailOffset,0U);
    fNextMessageId=1U;
    IOLog("LAVENDER RPM: TOC RX tail/head/fifo/len=%x/%x/%x/%x TX=%x/%x/%x/%x\n",
          fRings.rx.tailOffset,fRings.rx.headOffset,fRings.rx.fifoOffset,fRings.rx.length,
          fRings.tx.tailOffset,fRings.tx.headOffset,fRings.tx.fifoOffset,fRings.tx.length);
    if(!sendVersion()) {
        IOLog("LAVENDER RPM: failed to send GLINK VERSION\n");
        releaseResources();super::stop(provider);return false;
    }
    fStarted=true;gLavenderRPMGLINK=this;
    IOLog("LAVENDER RPM: GLINK transport ready (RAM=0x%llx size=0x%x, IRQ SPI %u polled)\n",
          (unsigned long long)LAVENDER_RPM_MSG_RAM_BASE,LAVENDER_RPM_MSG_RAM_SIZE,
          LAVENDER_RPM_IRQ_SPI);
    return true;
}

void LavenderRPMGLINK::stop(IOService *provider) {
    if(gLavenderRPMGLINK==this)gLavenderRPMGLINK=nullptr;
    fStarted=false;fChannelReady=false;
    releaseResources();
    super::stop(provider);
}

extern "C" bool lavender_rpm_vote_vdd_dig_low_svs(void) {
    for(uint32_t elapsed=0;elapsed<LAVENDER_GLINK_POLL_TIMEOUT_MS;elapsed++) {
        LavenderRPMGLINK *service=gLavenderRPMGLINK;
        if(service)return service->requestVddDigLowSVS();
        IOSleep(1);
    }
    IOLog("LAVENDER RPM: GLINK service did not start before QUP clock enable\n");
    return false;
}

extern "C" void lavender_register_rpm_glink_driver(void) {
    OSDictionary *personality=OSDictionary::withCapacity(4);
    OSArray *drivers=OSArray::withCapacity(1);
    OSString *className=OSString::withCStringNoCopy("LavenderRPMGLINK");
    OSString *provider=OSString::withCStringNoCopy("IOPlatformDevice");
    OSString *name=OSString::withCStringNoCopy("qcom,glink-rpm");
    OSString *category=OSString::withCStringNoCopy("LavenderRPMGLINK");
    if(!personality||!drivers||!className||!provider||!name||!category) {
        IOLog("LAVENDER RPM: personality allocation failed\n");
        if(personality)personality->release();if(drivers)drivers->release();
        if(className)className->release();if(provider)provider->release();
        if(name)name->release();if(category)category->release();
        return;
    }
    personality->setObject("IOClass",className);
    personality->setObject("IOProviderClass",provider);
    personality->setObject("IONameMatch",name);
    personality->setObject("IOMatchCategory",category);
    drivers->setObject(personality);
    bool ok=gIOCatalogue&&gIOCatalogue->addDrivers(drivers,true);
    drivers->release();personality->release();className->release();
    provider->release();name->release();category->release();
    if(!ok)IOLog("LAVENDER RPM: personality registration failed\n");
    else IOLog("LAVENDER RPM: qcom,glink-rpm personality registered\n");
}
#endif
