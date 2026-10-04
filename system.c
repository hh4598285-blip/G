#include "system.h"

#define VGA ((volatile uint16_t*)0xB8000)
#define W 80
#define H 25

static uint8_t row;
static uint8_t col;
static uint8_t attr = 0x0F;
static uint32_t ticks;
static uint8_t shift;
static uint8_t caps;

static inline void outb(uint16_t p, uint8_t v) {
    __asm__ volatile ("outb %0,%1" : : "a"(v), "Nd"(p));
}

static inline uint8_t inb(uint16_t p) {
    uint8_t v;
    __asm__ volatile ("inb %1,%0" : "=a"(v) : "Nd"(p));
    return v;
}

static void clear(void) {
    for (uint32_t y=0;y<H;y++) for (uint32_t x=0;x<W;x++) VGA[y*W+x]=((uint16_t)attr<<8)|' ';
    row=0; col=0;
}

static void newline(void) {
    col=0;
    if (++row>=H) {
        for (uint32_t y=1;y<H;y++) for (uint32_t x=0;x<W;x++) VGA[(y-1)*W+x]=VGA[y*W+x];
        for (uint32_t x=0;x<W;x++) VGA[(H-1)*W+x]=((uint16_t)attr<<8)|' ';
        row=H-1;
    }
}

static void put(char c) {
    if(c=='\n'){newline();return;}
    if(c=='\r'){col=0;return;}
    if(c=='\b'){if(col){--col;VGA[row*W+col]=((uint16_t)attr<<8)|' ';}return;}
    if(col>=W)newline();
    VGA[row*W+col]=((uint16_t)attr<<8)|(uint8_t)c;
    col++;
}

void system_print(const char* s){while(*s)put(*s++);}

void system_print_hex(uint32_t v){
    const char*d="0123456789ABCDEF";
    system_print("0x");
    for(int i=7;i>=0;i--)put(d[(v>>(i*4))&15]);
}

static char translate(uint8_t sc){
    static const char normal[]=" 1234567890-=\b\tqwertyuiop[]\n asdfghjkl;'`\\zxcvbnm,./";
    static const char upper[]=" !@#$%^&*()_+\b\tQWERTYUIOP{}\n ASDFGHJKL:\"~|ZXCVBNM<>?";
    if(sc<2 || sc>53)return 0;
    char c=(shift||caps)?upper[sc-1]:normal[sc-1];
    if(shift && sc>=2 && sc<=11 && caps)c=normal[sc-1];
    return c;
}

static void shell_command(void){
    system_print("\nGOS> ");
}

void system_init(void){
    clear();
    system_print("GOS SYSTEM CONSOLE\n");
    system_print("Kernel services online.\n");
    system_print("Keyboard service online.\n");
    system_print("Type commands in the system console.\n\nGOS> ");
    ticks=0;
}

void system_poll(void){
    ticks++;
    if(!(inb(0x64)&1))return;
    uint8_t sc=inb(0x60);
    if(sc==0x2A||sc==0x36){shift=1;return;}
    if(sc==0xAA||sc==0xB6){shift=0;return;}
    if(sc==0x3A){caps^=1;return;}
    if(sc&0x80)return;
    if(sc==0x1C){shell_command();return;}
    if(sc==0x0E){put('\b');return;}
    char c=translate(sc);
    if(c)put(c);
}

uint32_t system_ticks(void){return ticks;}
