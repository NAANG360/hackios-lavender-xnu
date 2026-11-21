#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "LavenderQUPI2C.h"
int main(){
 uint8_t tag[4]={};uint32_t n=0;
 assert(lavender_qup_encode_tag(0x62,false,false,2,true,tag,&n));
 assert(n==4&&tag[0]==0x81&&tag[1]==0xc4&&tag[2]==0x82&&tag[3]==2);
 assert(lavender_qup_encode_tag(0x62,true,true,5,true,tag,&n));
 assert(n==4&&tag[0]==0x81&&tag[1]==0xc5&&tag[2]==0x87&&tag[3]==5);
 uint8_t cmd[]={0x10,0x20},rsp[3]={};
 LavenderI2CMessage m[2]={{0x62,0,2,cmd},{0x62,LAVENDER_I2C_MESSAGE_READ,3,rsp}};
 LavenderQUPTransferSize z={};
 assert(lavender_qup_measure(m,2,&z));
 assert(z.outputBytes==10&&z.inputBytes==5&&z.txBytes==2&&z.rxBytes==3&&z.lastIsRead&&z.readMessages==1);
 uint8_t out[16]={};uint32_t written=0;
 assert(lavender_qup_build_output(m,2,out,sizeof(out),&written));
 const uint8_t expected[]={0x81,0xc4,0x82,0x02,0x10,0x20,0x81,0xc5,0x87,0x03};
 assert(written==sizeof(expected)&&memcmp(out,expected,sizeof(expected))==0);
 const uint8_t input[]={0x93,0x00,0xaa,0xbb,0xcc};
 assert(lavender_qup_consume_input(m,2,input,sizeof(input)));
 assert(rsp[0]==0xaa&&rsp[1]==0xbb&&rsp[2]==0xcc);

 /* Real NT36xxx path: 1-byte register write, 66-byte read, 2-byte QUP tag. */
 uint8_t reg=0x01, touchReport[66]={}, touchWire[68]={0x93,0x00};
 LavenderI2CMessage touch[2]={{0x62,0,1,&reg},{0x62,LAVENDER_I2C_MESSAGE_READ,66,touchReport}};
 assert(lavender_qup_measure(touch,2,&z));
 assert(z.outputBytes==9&&z.inputBytes==68&&z.readMessages==1);
 LavenderQUPFifoGeometry geometry={};enum LavenderQUPTransferMode mode=LAVENDER_QUP_MODE_FIFO;
 assert(lavender_qup_choose_mode(0,z.outputBytes,z.inputBytes,&mode,&geometry));
 assert(mode==LAVENDER_QUP_MODE_BLOCK);
 assert(geometry.outputBlockBytes==16&&geometry.inputBlockBytes==16);
 assert(geometry.outputFifoBytes==32&&geometry.inputFifoBytes==32);
 /* QUP_I2C_STATUS BUS_ACTIVE is bit 8; bus idle must be observed before reset. */
 assert(lavender_qup_bus_is_active(1U<<8));
 assert(!lavender_qup_bus_is_active(0U));
 assert(!lavender_qup_bus_is_active(1U<<9));
 /* A 68-byte Novatek read consumes four full 16-byte blocks and one 4-byte tail. */
 const uint32_t expectedTailBytes[]={16,16,16,16,4};
 const uint32_t expectedWords[]={4,4,4,4,1};
 for(uint32_t i=0;i<5;i++){
   uint32_t words=0;
   assert(lavender_qup_block_read_words(expectedTailBytes[i],16,&words));
   assert(words==expectedWords[i]);
 }
 uint32_t invalidWords=0;
 assert(!lavender_qup_block_read_words(17,16,&invalidWords));
 assert(!lavender_qup_block_read_words(4,0,&invalidWords));
 /* BLSP1 QUP1 DTS: qcom,i2c-dat is TLMM GPIO2; qcom,i2c-clk is GPIO3. */
 assert(LAVENDER_TLMM_QUP1_SDA_CTL==LAVENDER_TLMM_GPIO2_CTL);
 assert(LAVENDER_TLMM_QUP1_SCL_CTL==LAVENDER_TLMM_GPIO3_CTL);
 assert(LAVENDER_TLMM_QUP1_SDA_CTL==0x2000U&&LAVENDER_TLMM_QUP1_SCL_CTL==0x3000U);
 /* Native XNU pad settings match the pinned SDM660 pinctrl DTS. */
 const uint32_t tlmmMask=LAVENDER_TLMM_MUX_MASK|LAVENDER_TLMM_PULL_MASK|
                         LAVENDER_TLMM_DRIVE_MASK|LAVENDER_TLMM_OE;
 uint32_t oldControl=0xa5a55a5aU;
 uint32_t activeControl=lavender_qup_tlmm_control(oldControl,true);
 uint32_t sleepControl=lavender_qup_tlmm_control(oldControl,false);
 assert(((activeControl&LAVENDER_TLMM_MUX_MASK)>>LAVENDER_TLMM_MUX_SHIFT)==3U);
 assert((activeControl&LAVENDER_TLMM_PULL_MASK)==0U);
 assert((activeControl&LAVENDER_TLMM_DRIVE_MASK)==0U); /* 2mA */
 assert((activeControl&LAVENDER_TLMM_OE)==0U);
 assert((sleepControl&LAVENDER_TLMM_PULL_MASK)==LAVENDER_TLMM_PULL_UP);
 assert((sleepControl&LAVENDER_TLMM_DRIVE_MASK)==0U);
 uint32_t gpioControl=lavender_qup_tlmm_gpio_control(oldControl);
 assert(((gpioControl&LAVENDER_TLMM_MUX_MASK)>>LAVENDER_TLMM_MUX_SHIFT)==LAVENDER_TLMM_GPIO_FUNCTION);
 assert((gpioControl&LAVENDER_TLMM_PULL_MASK)==LAVENDER_TLMM_PULL_UP);
 assert((gpioControl&LAVENDER_TLMM_OE)==0U);
 assert((activeControl&~tlmmMask)==(oldControl&~tlmmMask));
 assert((sleepControl&~tlmmMask)==(oldControl&~tlmmMask));
 LavenderQUPRegisterCounts counts={};
 assert(lavender_qup_register_counts(&z,mode,&counts));
 assert(counts.mxInputCount==68&&counts.mxOutputCount==9);
 assert(counts.mxReadCount==0&&counts.mxWriteCount==0);
 assert(!counts.noInput&&!counts.noOutput);
 enum LavenderQUPTransferMode fifoMode=LAVENDER_QUP_MODE_FIFO;
 assert(lavender_qup_register_counts(&z,fifoMode,&counts));
 assert(counts.mxInputCount==0&&counts.mxOutputCount==0);
 assert(counts.mxReadCount==68&&counts.mxWriteCount==9);
 assert(!counts.noInput&&!counts.noOutput);
 /* Linux chooses BLOCK when a transfer exactly fills the FIFO. */
 assert(lavender_qup_choose_mode(0,1,32,&mode,&geometry));assert(mode==LAVENDER_QUP_MODE_BLOCK);
 assert(lavender_qup_choose_mode((2U<<7),z.outputBytes,z.inputBytes,&mode,&geometry));
 assert(mode==LAVENDER_QUP_MODE_FIFO&&geometry.inputFifoBytes==128);
 memset(touchWire+2,0x5a,66);
 assert(lavender_qup_consume_input(touch,2,touchWire,sizeof(touchWire)));
 assert(touchReport[0]==0x5a&&touchReport[65]==0x5a);
 assert(!lavender_qup_fifo_geometry((3U<<5),&geometry));

 assert(!lavender_qup_measure(m,0,&z));
 assert(!lavender_qup_encode_tag(0x80,false,true,1,true,tag,&n));
 m[0].length=0;assert(!lavender_qup_measure(m,2,&z));
 return 0;
}
