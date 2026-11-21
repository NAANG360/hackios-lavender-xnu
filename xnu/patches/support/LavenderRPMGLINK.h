#ifndef LAVENDER_RPM_GLINK_H
#define LAVENDER_RPM_GLINK_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LAVENDER_RPM_MSG_RAM_BASE 0x00778000ULL
#define LAVENDER_RPM_MSG_RAM_SIZE 0x00007000U
#define LAVENDER_RPM_TOC_SIZE 0x100U
#define LAVENDER_RPM_TOC_MAGIC 0x67727430U
#define LAVENDER_RPM_RX_FIFO_ID 0x72326170U
#define LAVENDER_RPM_TX_FIFO_ID 0x61703272U
#define LAVENDER_RPM_APCS_BASE 0x17911000ULL
#define LAVENDER_RPM_APCS_DOORBELL 0x008U
#define LAVENDER_RPM_APCS_DOORBELL_BIT 0x1U
#define LAVENDER_RPM_IRQ_SPI 168U
#define LAVENDER_RPM_CHANNEL "rpm_requests"
#define LAVENDER_RPM_CHANNEL_LENGTH 13U
#define LAVENDER_GLINK_VERSION 1U
#define LAVENDER_GLINK_FEATURES 0U
#define LAVENDER_GLINK_MAX_PACKET 256U
#define LAVENDER_GLINK_POLL_TIMEOUT_MS 3000U
#define LAVENDER_RPM_ACK_TIMEOUT_MS 500U

enum LavenderGlinkCommand {
    LAVENDER_GLINK_CMD_VERSION = 0,
    LAVENDER_GLINK_CMD_VERSION_ACK = 1,
    LAVENDER_GLINK_CMD_OPEN = 2,
    LAVENDER_GLINK_CMD_CLOSE = 3,
    LAVENDER_GLINK_CMD_OPEN_ACK = 4,
    LAVENDER_GLINK_CMD_CLOSE_ACK = 11,
    LAVENDER_GLINK_CMD_TX_DATA = 9,
    LAVENDER_GLINK_CMD_TX_DATA_CONT = 12,
    LAVENDER_GLINK_CMD_READ_NOTIF = 13
};

struct LavenderRPMRing {
    uint32_t tailOffset;
    uint32_t headOffset;
    uint32_t fifoOffset;
    uint32_t length;
};
struct LavenderRPMRings {
    struct LavenderRPMRing rx;
    struct LavenderRPMRing tx;
};

static inline uint16_t lavender_rpm_le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline uint32_t lavender_rpm_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline void lavender_rpm_put_le16(uint8_t *p, uint16_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
}
static inline void lavender_rpm_put_le32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static inline uint32_t lavender_rpm_align8(uint32_t v) {
    return (v + 7U) & ~7U;
}

/* RPM TOC entries point at an 8-byte {tail,head} descriptor followed by FIFO.
 * The TOC itself is excluded from the ring extents to prevent MMIO overruns. */
static inline bool lavender_rpm_parse_toc(const uint8_t *ram, uint32_t ramSize,
                                           struct LavenderRPMRings *rings) {
    if (!ram || !rings || ramSize < LAVENDER_RPM_TOC_SIZE) return false;
    const uint32_t tocOffset = ramSize - LAVENDER_RPM_TOC_SIZE;
    const uint8_t *toc = ram + tocOffset;
    if (lavender_rpm_le32(toc) != LAVENDER_RPM_TOC_MAGIC) return false;
    uint32_t count = lavender_rpm_le32(toc + 4);
    if (count > (LAVENDER_RPM_TOC_SIZE - 8U) / 12U) return false;
    bool haveRx=false, haveTx=false;
    struct LavenderRPMRings found = {};
    for (uint32_t i=0; i<count; ++i) {
        const uint8_t *entry=toc+8U+i*12U;
        uint32_t id=lavender_rpm_le32(entry);
        uint32_t offset=lavender_rpm_le32(entry+4);
        uint32_t length=lavender_rpm_le32(entry+8);
        if (length < 16U || (length & 7U) || (offset & 3U) || offset > tocOffset ||
            offset+8U > tocOffset || length > tocOffset-(offset+8U)) continue;
        struct LavenderRPMRing ring={offset,offset+4U,offset+8U,length};
        if (id==LAVENDER_RPM_RX_FIFO_ID) {
            if (haveRx) return false;
            found.rx=ring;
            haveRx=true;
        }
        if (id==LAVENDER_RPM_TX_FIFO_ID) {
            if (haveTx) return false;
            found.tx=ring;
            haveTx=true;
        }
    }
    if (!haveRx || !haveTx) return false;
    /* Reject aliased RX/TX descriptors or FIFOs before touching shared RAM. */
    const uint32_t rxEnd=found.rx.fifoOffset+found.rx.length;
    const uint32_t txEnd=found.tx.fifoOffset+found.tx.length;
    if (found.rx.tailOffset<txEnd && found.tx.tailOffset<rxEnd) return false;
    *rings=found;
    return true;
}
static inline bool lavender_rpm_ring_available(uint32_t head, uint32_t tail,
                                                 uint32_t length, uint32_t *available) {
    if (!available || !length || head>=length || tail>=length) return false;
    *available = head>=tail ? head-tail : length-tail+head;
    return true;
}
static inline bool lavender_rpm_ring_free(uint32_t head, uint32_t tail,
                                           uint32_t length, uint32_t *freeBytes) {
    uint32_t used;
    if (!freeBytes || !lavender_rpm_ring_available(head,tail,length,&used)) return false;
    *freeBytes = length-used-1U;
    return true;
}
static inline bool lavender_rpm_ring_copy_out(const uint8_t *ring, uint32_t length,
                                               uint32_t position, uint8_t *out,
                                               uint32_t count) {
    if (!ring || !out || !length || position>=length || count>length) return false;
    for (uint32_t i=0;i<count;i++) out[i]=ring[(position+i)%length];
    return true;
}
static inline bool lavender_rpm_ring_copy_in(uint8_t *ring, uint32_t length,
                                              uint32_t position, const uint8_t *in,
                                              uint32_t count) {
    if (!ring || !in || !length || position>=length || count>length) return false;
    for (uint32_t i=0;i<count;i++) ring[(position+i)%length]=in[i];
    return true;
}
static inline bool lavender_rpm_encode_glink_header(uint8_t *out, uint32_t capacity,
                                                      uint16_t command, uint16_t p1,
                                                      uint32_t p2) {
    if (!out || capacity<8U) return false;
    lavender_rpm_put_le16(out,command);
    lavender_rpm_put_le16(out+2,p1);
    lavender_rpm_put_le32(out+4,p2);
    return true;
}
static inline bool lavender_rpm_encode_glink_open(uint8_t *out, uint32_t capacity,
                                                   uint16_t localCid) {
    const uint32_t size=lavender_rpm_align8(8U+LAVENDER_RPM_CHANNEL_LENGTH);
    if (!out || capacity<size) return false;
    for (uint32_t i=0;i<size;i++) out[i]=0;
    if (!lavender_rpm_encode_glink_header(out,size,LAVENDER_GLINK_CMD_OPEN,
                                           localCid,LAVENDER_RPM_CHANNEL_LENGTH)) return false;
    const char name[LAVENDER_RPM_CHANNEL_LENGTH]="rpm_requests";
    for (uint32_t i=0;i<LAVENDER_RPM_CHANNEL_LENGTH;i++) out[8U+i]=(uint8_t)name[i];
    return true;
}
static inline bool lavender_rpm_encode_glink_tx_data(uint8_t *out, uint32_t capacity,
                                                      uint16_t localCid,
                                                      const uint8_t *payload,
                                                      uint32_t payloadLength,
                                                      uint32_t *encodedLength) {
    if (!out || !payload || !encodedLength || !payloadLength ||
        payloadLength>LAVENDER_GLINK_MAX_PACKET) return false;
    uint32_t size=lavender_rpm_align8(16U+payloadLength);
    if (capacity<size) return false;
    for (uint32_t i=0;i<size;i++) out[i]=0;
    if (!lavender_rpm_encode_glink_header(out,size,LAVENDER_GLINK_CMD_TX_DATA,
                                           localCid,0U)) return false;
    lavender_rpm_put_le32(out+8,payloadLength);
    lavender_rpm_put_le32(out+12,0U);
    for (uint32_t i=0;i<payloadLength;i++) out[16U+i]=payload[i];
    *encodedLength=size;
    return true;
}

/* RPM-SMD v0 request header. qcom,rpm-smd has no version register in the
 * pinned Lavender DTS, so Linux selects V0. GLINK/RPMSG is the outer envelope. */
#define LAVENDER_RPM_V0_SERVICE_REQUEST 0x00716572U /* bytes "req\\0" */
#define LAVENDER_RPM_V0_ACK_SERVICE 3U
#define LAVENDER_RPM_RESOURCE_RWXC 0x78637772U /* bytes "rwcx" */
#define LAVENDER_RPM_RESOURCE_ID_VDD_DIG 0U
#define LAVENDER_RPM_KVP_VLVL 0x6c766c76U /* bytes "vlvl" */
#define LAVENDER_RPM_LEVEL_LOW_SVS 64U
#define LAVENDER_RPM_V0_HEADER_SIZE 28U
#define LAVENDER_RPM_V0_REQUEST_SIZE 40U

static inline bool lavender_rpm_encode_v0_vdd_dig_vote(uint8_t *out, uint32_t capacity,
                                                        uint32_t messageId,
                                                        uint32_t level,
                                                        uint32_t *encodedLength) {
    if (!out || !encodedLength || capacity<LAVENDER_RPM_V0_REQUEST_SIZE ||
        messageId<2U || level>0xffffU) return false;
    for (uint32_t i=0;i<LAVENDER_RPM_V0_REQUEST_SIZE;i++) out[i]=0;
    lavender_rpm_put_le32(out+0,LAVENDER_RPM_V0_SERVICE_REQUEST);
    lavender_rpm_put_le32(out+4,32U);
    lavender_rpm_put_le32(out+8,messageId);
    lavender_rpm_put_le32(out+12,0U); /* active set */
    lavender_rpm_put_le32(out+16,LAVENDER_RPM_RESOURCE_RWXC);
    lavender_rpm_put_le32(out+20,LAVENDER_RPM_RESOURCE_ID_VDD_DIG);
    lavender_rpm_put_le32(out+24,12U);
    lavender_rpm_put_le32(out+28,LAVENDER_RPM_KVP_VLVL);
    lavender_rpm_put_le32(out+32,4U);
    lavender_rpm_put_le32(out+36,level);
    *encodedLength=LAVENDER_RPM_V0_REQUEST_SIZE;
    return true;
}
static inline bool lavender_rpm_decode_v0_ack(const uint8_t *data, uint32_t length,
                                               uint32_t *messageId, int32_t *status) {
    if (!data || !messageId || !status || length<20U) return false;
    *messageId=lavender_rpm_le32(data+16);
    uint32_t requestLength=lavender_rpm_le32(data+4);
    *status=(lavender_rpm_le32(data)==LAVENDER_RPM_V0_ACK_SERVICE ||
             requestLength==12U) ? 0 : -1;
    return true;
}
#endif
