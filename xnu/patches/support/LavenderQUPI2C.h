#ifndef _LAVENDER_QUP_I2C_H_
#define _LAVENDER_QUP_I2C_H_
#include <stdint.h>
#include <stddef.h>
#define LAVENDER_QUP_I2C_BASE 0x0c175000ULL
#define LAVENDER_QUP_I2C_LENGTH 0x600U
#define LAVENDER_QUP_I2C_ADDRESS 0x62U
#define LAVENDER_QUP_MAX_MESSAGES 8U
#define LAVENDER_QUP_MAX_MESSAGE 255U
#define LAVENDER_QUP_MAX_FIFO_BYTES 2048U
#define LAVENDER_I2C_MESSAGE_READ 0x0001U
#define LAVENDER_I2C_TRANSFER_FUNCTION "lavender-i2c-transfer"
struct LavenderI2CMessage { uint16_t address; uint16_t flags; uint32_t length; uint8_t *data; };
struct LavenderI2CTransfer { struct LavenderI2CMessage *messages; uint32_t messageCount; uint32_t timeoutMilliseconds; };
struct LavenderQUPTransferSize { uint32_t outputBytes, inputBytes, txBytes, rxBytes; bool lastIsRead; };
static inline bool lavender_qup_encode_tag(uint16_t address, bool read, bool last, uint8_t length, bool start, uint8_t bytes[4], uint32_t *count) {
    uint8_t command;
    if (address > 0x7f || length == 0 || bytes == NULL || count == NULL) return false;
    command = read ? (last ? 0x87 : 0x85) : (last ? 0x83 : 0x82);
    /* 0x87 is QUP_TAG2_DATA_READ_N_STOP; 0x83 is WRITE_N_STOP. */
    if (start) { bytes[0]=0x81; /* QUP_TAG2_START */ bytes[1]=(uint8_t)((address<<1)|(read?1U:0U)); bytes[2]=command; bytes[3]=length; *count=4; }
    else { bytes[0]=command; bytes[1]=length; *count=2; }
    return true;
}
static inline bool lavender_qup_measure(const struct LavenderI2CMessage *messages, uint32_t count, struct LavenderQUPTransferSize *size) {
    uint32_t i; uint64_t output=0,input=0,tx=0,rx=0;
    if (!messages || !size || count==0 || count>LAVENDER_QUP_MAX_MESSAGES) return false;
    for (i=0;i<count;i++) {
        const struct LavenderI2CMessage *m=&messages[i];
        if (m->address>0x7f || !m->length || m->length>LAVENDER_QUP_MAX_MESSAGE || !m->data || (m->flags & ~LAVENDER_I2C_MESSAGE_READ)) return false;
        output+=4;
        if (m->flags & LAVENDER_I2C_MESSAGE_READ) { input+=m->length+2; rx+=m->length; }
        else { output+=m->length; tx+=m->length; }
        if (output>LAVENDER_QUP_MAX_FIFO_BYTES || input>LAVENDER_QUP_MAX_FIFO_BYTES) return false;
    }
    size->outputBytes=(uint32_t)output; size->inputBytes=(uint32_t)input;
    size->txBytes=(uint32_t)tx; size->rxBytes=(uint32_t)rx;
    size->lastIsRead=(messages[count-1].flags & LAVENDER_I2C_MESSAGE_READ)!=0;
    return true;
}
static inline bool lavender_qup_build_output(const struct LavenderI2CMessage *messages, uint32_t count, uint8_t *output, uint32_t capacity, uint32_t *written) {
    uint32_t i,cursor=0;
    if (!output || !written || !messages || !count || count>LAVENDER_QUP_MAX_MESSAGES) return false;
    for (i=0;i<count;i++) {
        uint8_t tag[4]; uint32_t tagBytes=0,j; const struct LavenderI2CMessage *m=&messages[i];
        bool read=(m->flags & LAVENDER_I2C_MESSAGE_READ)!=0;
        if (!lavender_qup_encode_tag(m->address,read,i==count-1,(uint8_t)m->length,true,tag,&tagBytes) || cursor>capacity || tagBytes>capacity-cursor) return false;
        for (j=0;j<tagBytes;j++) output[cursor++]=tag[j];
        if (!read) { if (cursor>capacity || m->length>capacity-cursor) return false; for (j=0;j<m->length;j++) output[cursor++]=m->data[j]; }
    }
    *written=cursor; return true;
}
static inline bool lavender_qup_consume_input(const struct LavenderI2CMessage *messages, uint32_t count, const uint8_t *input, uint32_t inputLength) {
    uint32_t i,cursor=0;
    if (!messages || !input || !count) return false;
    for (i=0;i<count;i++) {
        const struct LavenderI2CMessage *m=&messages[i]; uint32_t j;
        if (!(m->flags & LAVENDER_I2C_MESSAGE_READ)) continue;
        if (cursor>inputLength || inputLength-cursor<2U+m->length) return false;
        cursor+=2;
        for (j=0;j<m->length;j++) m->data[j]=input[cursor++];
    }
    return cursor==inputLength;
}
#endif
