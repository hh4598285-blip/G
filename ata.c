#include "ata.h"

#define ATA_DATA 0x1F0
#define ATA_ERROR 0x1F1
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA0 0x1F3
#define ATA_LBA1 0x1F4
#define ATA_LBA2 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_COMMAND 0x1F7
#define ATA_ALTSTATUS 0x3F6

#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_SR_BSY 0x80
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

static int present;
static uint32_t sectors;
static char model[41];

static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint16_t inw(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outw(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static void io_wait(void){inb(ATA_ALTSTATUS);inb(ATA_ALTSTATUS);inb(ATA_ALTSTATUS);inb(ATA_ALTSTATUS);}

static int wait_ready(int want_drq){
    for(uint32_t i=0;i<1000000;i++){
        uint8_t s=inb(ATA_STATUS);
        if(s&ATA_SR_ERR)return -1;
        if(!(s&ATA_SR_BSY) && (!want_drq || (s&ATA_SR_DRQ)))return 0;
    }
    return -1;
}

int ata_init(void){
    present=0;sectors=0;
    for(int i=0;i<40;i++)model[i]=' ';
    model[40]=0;

    outb(ATA_DRIVE,0xA0);
    io_wait();
    outb(ATA_SECCOUNT,0);
    outb(ATA_LBA0,0);
    outb(ATA_LBA1,0);
    outb(ATA_LBA2,0);
    outb(ATA_COMMAND,ATA_CMD_IDENTIFY);
    io_wait();

    uint8_t s=inb(ATA_STATUS);
    if(!s || wait_ready(0)<0)return 0;

    if((inb(ATA_LBA1)!=0)||(inb(ATA_LBA2)!=0))return 0;
    if(wait_ready(1)<0)return 0;

    uint16_t id[256];
    for(int i=0;i<256;i++)id[i]=inw(ATA_DATA);

    if(!(id[0]&0x8000u) && id[49]&0x0200u){
        sectors=(uint32_t)id[60]|((uint32_t)id[61]<<16);
        for(int i=0;i<20;i++){
            uint16_t w=id[27+i];
            model[i*2]=(char)(w>>8);
            model[i*2+1]=(char)(w&0xFF);
        }
        model[40]=0;
        present=1;
    }
    return present;
}

int ata_present(void){return present;}

uint32_t ata_sectors(void){return sectors;}

const char* ata_model(void){return model;}

static int select_lba(uint32_t lba){
    if(!present || lba>=sectors || (lba&0xF0000000u))return -1;
    outb(ATA_DRIVE,0xE0|((lba>>24)&0x0F));
    outb(ATA_SECCOUNT,1);
    outb(ATA_LBA0,(uint8_t)lba);
    outb(ATA_LBA1,(uint8_t)(lba>>8));
    outb(ATA_LBA2,(uint8_t)(lba>>16));
    return 0;
}

int ata_read28(uint32_t lba,uint8_t*buffer){
    if(!buffer||select_lba(lba)<0)return -1;
    outb(ATA_COMMAND,ATA_CMD_READ);
    if(wait_ready(1)<0)return -1;
    for(int i=0;i<256;i++){
        uint16_t w=inw(ATA_DATA);
        buffer[i*2]=(uint8_t)w;
        buffer[i*2+1]=(uint8_t)(w>>8);
    }
    return 0;
}

int ata_write28(uint32_t lba,const uint8_t*buffer){
    if(!buffer||select_lba(lba)<0)return -1;
    outb(ATA_COMMAND,ATA_CMD_WRITE);
    if(wait_ready(1)<0)return -1;
    for(int i=0;i<256;i++){
        uint16_t w=(uint16_t)buffer[i*2]|((uint16_t)buffer[i*2+1]<<8);
        outw(ATA_DATA,w);
    }
    io_wait();
    return wait_ready(0);
}
