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
 LavenderQUPTransferSize z={};assert(lavender_qup_measure(m,2,&z));
 assert(z.outputBytes==10&&z.inputBytes==5&&z.txBytes==2&&z.rxBytes==3&&z.lastIsRead);
 uint8_t out[16]={};uint32_t written=0;assert(lavender_qup_build_output(m,2,out,sizeof(out),&written));
 const uint8_t expected[]={0x81,0xc4,0x82,0x02,0x10,0x20,0x81,0xc5,0x87,0x03};
 assert(written==sizeof(expected)&&memcmp(out,expected,sizeof(expected))==0);
 const uint8_t input[]={0x93,0x00,0xaa,0xbb,0xcc};
 assert(lavender_qup_consume_input(m,2,input,sizeof(input)));
 assert(rsp[0]==0xaa&&rsp[1]==0xbb&&rsp[2]==0xcc);
 assert(!lavender_qup_measure(m,0,&z));assert(!lavender_qup_encode_tag(0x80,false,true,1,true,tag,&n));
 m[0].length=0;assert(!lavender_qup_measure(m,2,&z));return 0;
}
