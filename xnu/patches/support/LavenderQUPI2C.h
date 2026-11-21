#ifndef _LAVENDER_QUP_I2C_H_
#define _LAVENDER_QUP_I2C_H_
#include <stdint.h>
#include <stddef.h>
#define LAVENDER_QUP_I2C_BASE 0x0c175000ULL
/* SDM660 TLMM SOUTH tile: gpio2/gpio3 are the BLSP1 QUP1 I2C pads. */
#define LAVENDER_TLMM_SOUTH_BASE 0x03100000ULL
#define LAVENDER_TLMM_SOUTH_LENGTH 0x00004000U
#define LAVENDER_TLMM_GPIO2_CTL 0x00002000U
#define LAVENDER_TLMM_GPIO3_CTL 0x00003000U
/* BLSP1 QUP1 DTS maps data to GPIO2 and clock to GPIO3. */
#define LAVENDER_TLMM_QUP1_SDA_CTL LAVENDER_TLMM_GPIO2_CTL
#define LAVENDER_TLMM_QUP1_SCL_CTL LAVENDER_TLMM_GPIO3_CTL
#define LAVENDER_TLMM_MUX_SHIFT 2U
#define LAVENDER_TLMM_MUX_MASK (0x0fU << LAVENDER_TLMM_MUX_SHIFT)
#define LAVENDER_TLMM_PULL_MASK 0x03U
#define LAVENDER_TLMM_DRIVE_MASK (0x07U << 6)
#define LAVENDER_TLMM_OE (1U << 9)
#define LAVENDER_TLMM_BLSP_I2C1_FUNCTION 3U
#define LAVENDER_TLMM_GPIO_FUNCTION 0U
#define LAVENDER_TLMM_NO_PULL 0U
#define LAVENDER_TLMM_PULL_UP 3U
#define LAVENDER_QUP_I2C_LENGTH 0x600U
#define LAVENDER_QUP_I2C_ADDRESS 0x62U
#define LAVENDER_QUP_BUS_ACTIVE_FLAG (1U << 8)
static inline bool lavender_qup_bus_is_active(uint32_t masterStatus) {
    return (masterStatus & LAVENDER_QUP_BUS_ACTIVE_FLAG) != 0;
}
#define LAVENDER_QUP_MAX_MESSAGES 8U
#define LAVENDER_QUP_MAX_MESSAGE 255U
#define LAVENDER_QUP_MAX_FIFO_BYTES 2048U
#define LAVENDER_QUP_DEFAULT_TIMEOUT_MS 500U
#define LAVENDER_QUP_MAX_TIMEOUT_MS 5000U
static inline uint32_t lavender_qup_clamp_timeout(uint32_t requested) {
    if (!requested) return LAVENDER_QUP_DEFAULT_TIMEOUT_MS;
    return requested > LAVENDER_QUP_MAX_TIMEOUT_MS
        ? LAVENDER_QUP_MAX_TIMEOUT_MS : requested;
}
#define LAVENDER_I2C_MESSAGE_READ 0x0001U
#define LAVENDER_I2C_TRANSFER_FUNCTION "lavender-i2c-transfer"

struct LavenderI2CMessage { uint16_t address; uint16_t flags; uint32_t length; uint8_t *data; };
struct LavenderI2CTransfer { struct LavenderI2CMessage *messages; uint32_t messageCount; uint32_t timeoutMilliseconds; };
struct LavenderQUPTransferSize { uint32_t outputBytes, inputBytes, txBytes, rxBytes, readMessages; bool lastIsRead; };
struct LavenderQUPFifoGeometry { uint32_t inputFifoBytes, outputFifoBytes, inputBlockBytes, outputBlockBytes; };
enum LavenderQUPTransferMode { LAVENDER_QUP_MODE_FIFO=0, LAVENDER_QUP_MODE_BLOCK=1 };

struct LavenderQUPRegisterCounts {
    uint32_t mxInputCount, mxOutputCount, mxReadCount, mxWriteCount;
    bool noInput, noOutput;
};

/* SDM660 PINGROUP(2/3): mux bits 2..5, pull 0..1, drive 6..8,
 * output-enable bit 9. DTS active: function index 3, 2mA, no pull.
 * DTS sleep: same mux, 2mA, pull-up. */
static inline uint32_t lavender_qup_tlmm_control(uint32_t oldValue, bool active) {
    uint32_t mask = LAVENDER_TLMM_MUX_MASK |
                    LAVENDER_TLMM_PULL_MASK |
                    LAVENDER_TLMM_DRIVE_MASK |
                    LAVENDER_TLMM_OE;
    uint32_t value = oldValue & ~mask;
    value |= (LAVENDER_TLMM_BLSP_I2C1_FUNCTION << LAVENDER_TLMM_MUX_SHIFT);
    if (!active) value |= LAVENDER_TLMM_PULL_UP;
    return value;
}
static inline uint32_t lavender_qup_tlmm_gpio_control(uint32_t oldValue) {
    uint32_t mask = LAVENDER_TLMM_MUX_MASK |
                    LAVENDER_TLMM_PULL_MASK |
                    LAVENDER_TLMM_DRIVE_MASK |
                    LAVENDER_TLMM_OE;
    uint32_t value = oldValue & ~mask;
    value |= (LAVENDER_TLMM_GPIO_FUNCTION << LAVENDER_TLMM_MUX_SHIFT) |
             LAVENDER_TLMM_PULL_UP;
    return value;
}

/* QUP v2 uses MX_INPUT/OUTPUT_COUNT for BLOCK mode and MX_READ/WRITE_COUNT
 * for FIFO mode. The two register pairs are not interchangeable. */
static inline bool lavender_qup_register_counts(const struct LavenderQUPTransferSize *size,
                                                  enum LavenderQUPTransferMode mode,
                                                  struct LavenderQUPRegisterCounts *counts) {
    if (!size || !counts ||
        (mode != LAVENDER_QUP_MODE_FIFO && mode != LAVENDER_QUP_MODE_BLOCK) ||
        size->outputBytes > LAVENDER_QUP_MAX_FIFO_BYTES ||
        size->inputBytes > LAVENDER_QUP_MAX_FIFO_BYTES) return false;
    counts->noInput = size->inputBytes == 0;
    counts->noOutput = size->outputBytes == 0;
    if (mode == LAVENDER_QUP_MODE_BLOCK) {
        counts->mxInputCount = size->inputBytes;
        counts->mxOutputCount = size->outputBytes;
        counts->mxReadCount = 0;
        counts->mxWriteCount = 0;
    } else {
        counts->mxInputCount = 0;
        counts->mxOutputCount = 0;
        counts->mxReadCount = size->inputBytes;
        counts->mxWriteCount = size->outputBytes;
    }
    return true;
}

static inline bool lavender_qup_fifo_geometry(uint32_t ioModes, struct LavenderQUPFifoGeometry *g) {
    static const uint8_t blockBytes[4]={16,16,32,0}, fifoBlocks[4]={2,4,8,16};
    if (!g) return false;
    g->outputBlockBytes=blockBytes[ioModes&3U];
    g->outputFifoBytes=g->outputBlockBytes*fifoBlocks[(ioModes>>2)&3U];
    g->inputBlockBytes=blockBytes[(ioModes>>5)&3U];
    g->inputFifoBytes=g->inputBlockBytes*fifoBlocks[(ioModes>>7)&3U];
    return g->inputBlockBytes && g->outputBlockBytes &&
           g->inputFifoBytes && g->outputFifoBytes &&
           g->inputFifoBytes<=LAVENDER_QUP_MAX_FIFO_BYTES &&
           g->outputFifoBytes<=LAVENDER_QUP_MAX_FIFO_BYTES;
}
static inline bool lavender_qup_choose_mode(uint32_t ioModes, uint32_t outputBytes, uint32_t inputBytes,
                                             enum LavenderQUPTransferMode *mode,
                                             struct LavenderQUPFifoGeometry *g) {
    if (!mode || !lavender_qup_fifo_geometry(ioModes,g) ||
        outputBytes>LAVENDER_QUP_MAX_FIFO_BYTES || inputBytes>LAVENDER_QUP_MAX_FIFO_BYTES) return false;
    *mode=(outputBytes>=g->outputFifoBytes || inputBytes>=g->inputFifoBytes)
        ? LAVENDER_QUP_MODE_BLOCK : LAVENDER_QUP_MODE_FIFO;
    return true;
}
/* Number of 32-bit FIFO words required to drain a partial final block.
 * QUP's last BLOCK-mode read may be shorter than the hardware block size. */
static inline bool lavender_qup_block_read_words(uint32_t bytes, uint32_t blockBytes,
                                                  uint32_t *words) {
    if (!words || !blockBytes || (blockBytes & 3U) || bytes > blockBytes) return false;
    *words = (bytes + 3U) / 4U;
    return true;
}

static inline bool lavender_qup_encode_tag(uint16_t address, bool read, bool last, uint8_t length, bool start, uint8_t bytes[4], uint32_t *count) {
    uint8_t command;
    if (address>0x7f || length==0 || bytes==NULL || count==NULL) return false;
    command=read?(last?0x87:0x85):(last?0x83:0x82);
    /* QUP_TAG2_START=0x81; DATA_WRITE=0x82; DATA_WRITE_N_STOP=0x83;
     * DATA_READ=0x85; DATA_READ_N_STOP=0x87. */
    if (start) { bytes[0]=0x81; bytes[1]=(uint8_t)((address<<1)|(read?1U:0U)); bytes[2]=command; bytes[3]=length; *count=4; }
    else { bytes[0]=command; bytes[1]=length; *count=2; }
    return true;
}
static inline bool lavender_qup_measure(const struct LavenderI2CMessage *messages, uint32_t count, struct LavenderQUPTransferSize *size) {
    uint32_t i; uint64_t output=0,input=0,tx=0,rx=0,reads=0;
    if (!messages || !size || count==0 || count>LAVENDER_QUP_MAX_MESSAGES) return false;
    for (i=0;i<count;i++) {
        const struct LavenderI2CMessage *m=&messages[i];
        if (m->address>0x7f || !m->length || m->length>LAVENDER_QUP_MAX_MESSAGE || !m->data ||
            (m->flags & ~LAVENDER_I2C_MESSAGE_READ)) return false;
        output+=4;
        if (m->flags & LAVENDER_I2C_MESSAGE_READ) { input+=m->length+2; rx+=m->length; reads++; }
        else { output+=m->length; tx+=m->length; }
        if (output>LAVENDER_QUP_MAX_FIFO_BYTES || input>LAVENDER_QUP_MAX_FIFO_BYTES) return false;
    }
    size->outputBytes=(uint32_t)output; size->inputBytes=(uint32_t)input;
    size->txBytes=(uint32_t)tx; size->rxBytes=(uint32_t)rx; size->readMessages=(uint32_t)reads;
    size->lastIsRead=(messages[count-1].flags & LAVENDER_I2C_MESSAGE_READ)!=0;
    return true;
}
static inline bool lavender_qup_build_output(const struct LavenderI2CMessage *messages, uint32_t count, uint8_t *output, uint32_t capacity, uint32_t *written) {
    uint32_t i,cursor=0;
    if (!output || !written || !messages || !count || count>LAVENDER_QUP_MAX_MESSAGES) return false;
    for (i=0;i<count;i++) {
        uint8_t tag[4]; uint32_t tagBytes=0,j; const struct LavenderI2CMessage *m=&messages[i];
        bool read=(m->flags & LAVENDER_I2C_MESSAGE_READ)!=0;
        if (m->address>0x7f || !m->length || m->length>LAVENDER_QUP_MAX_MESSAGE || !m->data ||
            (m->flags & ~LAVENDER_I2C_MESSAGE_READ)) return false;
        if (!lavender_qup_encode_tag(m->address,read,i==count-1,(uint8_t)m->length,true,tag,&tagBytes) ||
            cursor>capacity || tagBytes>capacity-cursor) return false;
        for (j=0;j<tagBytes;j++) output[cursor++]=tag[j];
        if (!read) { if (cursor>capacity || m->length>capacity-cursor) return false; for (j=0;j<m->length;j++) output[cursor++]=m->data[j]; }
    }
    *written=cursor; return true;
}
static inline bool lavender_qup_consume_input(const struct LavenderI2CMessage *messages, uint32_t count, const uint8_t *input, uint32_t inputLength) {
    uint32_t i,cursor=0;
    if (!messages || !count || (inputLength && !input)) return false;
    for (i=0;i<count;i++) {
        const struct LavenderI2CMessage *m=&messages[i]; uint32_t j;
        if (!(m->flags & LAVENDER_I2C_MESSAGE_READ)) continue;
        if (!input || cursor>inputLength || inputLength-cursor<2U+m->length) return false;
        cursor+=2;
        for (j=0;j<m->length;j++) m->data[j]=input[cursor++];
    }
    return cursor==inputLength;
}
#endif
