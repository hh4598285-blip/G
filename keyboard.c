#include "keyboard.h"
#define KBD_DATA 0x60
#define KBD_STATUS 0x64
#define BUFFER_SIZE 256
static volatile char buffer[BUFFER_SIZE];
static volatile uint32_t head,tail,keys;
static uint8_t shift,caps;
static const char normal_map[128]={
0,27,49,50,51,52,53,54,55,56,57,48,45,61,8,9,
113,119,101,114,116,121,117,105,111,112,91,93,10,0,97,115,
100,102,103,104,106,107,108,59,39,96,0,92,122,120,99,118,
98,110,109,44,46,47,0,42,0,32,0
};
static const char shift_map[128]={
0,27,33,64,35,36,37,94,38,42,40,41,95,43,8,9,
81,87,69,82,84,89,85,73,79,80,123,125,10,0,65,83,
68,70,71,72,74,75,76,58,34,126,0,124,90,88,67,86,
66,78,77,60,62,63,0,42,0,32,0
};
static inline uint8_t inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
void keyboard_init(void){head=tail=keys=0;shift=caps=0;}
static void push(char c){uint32_t n=(head+1)%BUFFER_SIZE;if(n!=tail){buffer[head]=c;head=n;}}
void keyboard_irq(void){
 if(!(inb(KBD_STATUS)&1u))return; uint8_t sc=inb(KBD_DATA);
 if(sc==0x2A||sc==0x36){shift=1;return;} if(sc==0xAA||sc==0xB6){shift=0;return;}
 if(sc==0x3A){caps^=1u;return;} if(sc&0x80u)return;
 if(sc<128){
  char c=normal_map[sc];
  int letter=(sc>=0x10&&sc<=0x19)||(sc>=0x1E&&sc<=0x26)||(sc>=0x2C&&sc<=0x32);
  if(shift)c=shift_map[sc];
  if(caps&&letter)c=shift_map[sc];
  if(shift&&caps&&letter)c=normal_map[sc];
  if(c){push(c);++keys;}
 }
}
int keyboard_read(char*out){if(tail==head)return 0;*out=buffer[tail];tail=(tail+1)%BUFFER_SIZE;return 1;}
uint32_t keyboard_key_count(void){return keys;}
