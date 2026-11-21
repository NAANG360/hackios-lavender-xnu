#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "LavenderRPMGLINK.h"

int main() {
    uint8_t ram[LAVENDER_RPM_MSG_RAM_SIZE] = {};
    const uint32_t toc=LAVENDER_RPM_MSG_RAM_SIZE-LAVENDER_RPM_TOC_SIZE;
    lavender_rpm_put_le32(ram+toc,LAVENDER_RPM_TOC_MAGIC);
    lavender_rpm_put_le32(ram+toc+4,2);
    lavender_rpm_put_le32(ram+toc+8,LAVENDER_RPM_TX_FIFO_ID);
    lavender_rpm_put_le32(ram+toc+12,0x100);
    lavender_rpm_put_le32(ram+toc+16,0x200);
    lavender_rpm_put_le32(ram+toc+20,LAVENDER_RPM_RX_FIFO_ID);
    lavender_rpm_put_le32(ram+toc+24,0x400);
    lavender_rpm_put_le32(ram+toc+28,0x200);
    LavenderRPMRings rings={};
    assert(lavender_rpm_parse_toc(ram,sizeof(ram),&rings));
    assert(rings.tx.tailOffset==0x100&&rings.tx.headOffset==0x104);
    assert(rings.tx.fifoOffset==0x108&&rings.tx.length==0x200);
    assert(rings.rx.tailOffset==0x400&&rings.rx.headOffset==0x404);
    assert(rings.rx.fifoOffset==0x408&&rings.rx.length==0x200);

    uint32_t available=0,freeBytes=0;
    assert(lavender_rpm_ring_available(20,12,64,&available)&&available==8);
    assert(lavender_rpm_ring_available(4,60,64,&available)&&available==8);
    assert(lavender_rpm_ring_free(4,60,64,&freeBytes)&&freeBytes==55);
    assert(!lavender_rpm_ring_available(64,0,64,&available));
    uint8_t ring[16]={},payload[]={1,2,3,4,5,6};
    uint8_t copied[6]={};
    assert(lavender_rpm_ring_copy_in(ring,sizeof(ring),13,payload,sizeof(payload)));
    assert(lavender_rpm_ring_copy_out(ring,sizeof(ring),13,copied,sizeof(copied)));
    assert(memcmp(payload,copied,sizeof(payload))==0);

    uint8_t command[32]={};
    assert(lavender_rpm_encode_glink_header(command,sizeof(command),
        LAVENDER_GLINK_CMD_VERSION,1,0));
    assert(lavender_rpm_le16(command)==LAVENDER_GLINK_CMD_VERSION);
    assert(lavender_rpm_le16(command+2)==1);
    assert(lavender_rpm_le32(command+4)==0);
    assert(lavender_rpm_encode_glink_open(command,sizeof(command),1));
    assert(lavender_rpm_le16(command)==LAVENDER_GLINK_CMD_OPEN);
    assert(lavender_rpm_le16(command+2)==1);
    assert(lavender_rpm_le32(command+4)==LAVENDER_RPM_CHANNEL_LENGTH);
    assert(memcmp(command+8,"rpm_requests",13)==0);
    assert(lavender_rpm_align8(21)==24);

    uint8_t rpmRequest[LAVENDER_RPM_V0_REQUEST_SIZE]={};
    uint32_t requestLength=0;
    assert(lavender_rpm_encode_v0_vdd_dig_vote(rpmRequest,sizeof(rpmRequest),
        2,LAVENDER_RPM_LEVEL_LOW_SVS,&requestLength));
    assert(requestLength==40);
    assert(lavender_rpm_le32(rpmRequest+0)==LAVENDER_RPM_V0_SERVICE_REQUEST);
    assert(lavender_rpm_le32(rpmRequest+4)==32);
    assert(lavender_rpm_le32(rpmRequest+8)==2);
    assert(lavender_rpm_le32(rpmRequest+12)==0); /* active set */
    assert(lavender_rpm_le32(rpmRequest+16)==LAVENDER_RPM_RESOURCE_RWXC);
    assert(lavender_rpm_le32(rpmRequest+20)==LAVENDER_RPM_RESOURCE_ID_VDD_DIG);
    assert(lavender_rpm_le32(rpmRequest+24)==12);
    assert(lavender_rpm_le32(rpmRequest+28)==LAVENDER_RPM_KVP_VLVL);
    assert(lavender_rpm_le32(rpmRequest+32)==4);
    assert(lavender_rpm_le32(rpmRequest+36)==64); /* LOW_SVS */
    uint8_t glinkData[64]={};
    uint32_t glinkLength=0;
    assert(lavender_rpm_encode_glink_tx_data(glinkData,sizeof(glinkData),1,
        rpmRequest,requestLength,&glinkLength));
    assert(glinkLength==56);
    assert(lavender_rpm_le16(glinkData)==LAVENDER_GLINK_CMD_TX_DATA);
    assert(lavender_rpm_le16(glinkData+2)==1);
    assert(lavender_rpm_le32(glinkData+8)==40);
    assert(lavender_rpm_le32(glinkData+12)==0);
    assert(memcmp(glinkData+16,rpmRequest,40)==0);

    uint8_t ack[20]={};
    lavender_rpm_put_le32(ack,LAVENDER_RPM_V0_ACK_SERVICE);
    lavender_rpm_put_le32(ack+16,2);
    uint32_t ackId=0; int32_t ackStatus=-1;
    assert(lavender_rpm_decode_v0_ack(ack,sizeof(ack),&ackId,&ackStatus));
    assert(ackId==2&&ackStatus==0);
    lavender_rpm_put_le32(ack,0);
    lavender_rpm_put_le32(ack+4,12);
    assert(lavender_rpm_decode_v0_ack(ack,sizeof(ack),&ackId,&ackStatus));
    assert(ackStatus==0); /* Linux accepts an empty V0 error payload as success. */
    lavender_rpm_put_le32(ack+4,20);
    assert(lavender_rpm_decode_v0_ack(ack,sizeof(ack),&ackId,&ackStatus));
    assert(ackStatus==-1);

    /* Duplicate required ring IDs and overlapping ring regions are malformed. */
    lavender_rpm_put_le32(ram+toc+4,3);
    lavender_rpm_put_le32(ram+toc+32,LAVENDER_RPM_RX_FIFO_ID);
    lavender_rpm_put_le32(ram+toc+36,0x800);
    lavender_rpm_put_le32(ram+toc+40,0x200);
    assert(!lavender_rpm_parse_toc(ram,sizeof(ram),&rings));

    lavender_rpm_put_le32(ram+toc+4,2);
    lavender_rpm_put_le32(ram+toc+20,LAVENDER_RPM_RX_FIFO_ID);
    lavender_rpm_put_le32(ram+toc+24,0x280);
    lavender_rpm_put_le32(ram+toc+28,0x200);
    assert(!lavender_rpm_parse_toc(ram,sizeof(ram),&rings));

    lavender_rpm_put_le32(ram+toc,0);
    assert(!lavender_rpm_parse_toc(ram,sizeof(ram),&rings));
    return 0;
}
