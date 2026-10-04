#include "keyboard.h"

#define KBD_DATA 0x60
#define KBD_STATUS 0x64
#define BUFFER_SIZE 256

static volatile char buffer[BUFFER_SIZE];
static volatile uint32_t head;
static volatile uint32_t tail;
static uint32_t keys;
static uint8_t shift;
static uint8_t caps;

static const char normal_map[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=','','	',
    'q','w','e','r','t','y','u','i','o','p','[',']','
',0,'a','s',
    'd','f','g','h','j','k','l',';','\'',96,0,92,'z','x','c','v',
    'b','n','m',',','.','/',0,'*',0,' ',0
};

static const char shift_map[128] = {
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+','','	',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','
',0,'A','S',
    'D','F','G','H','J','K','L',':','"', '~',0,'|','Z','X','C','V',
    'B','N','M','<','>','?',0,'*',0,' ',0
};

static inline uint8_t inb(uint16_t p) { uint8_t v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(p)); return v; }

void keyboard_init(void) { head = tail = keys = 0; shift = 0; caps = 0; }

static void push(char c) {
    uint32_t next = (head + 1) % BUFFER_SIZE;
    if (next != tail) { buffer[head] = c; head = next; }
}

void keyboard_irq(void) {
    if (!(inb(KBD_STATUS) & 1u)) return;
    uint8_t sc = inb(KBD_DATA);

    if (sc == 0x2A || sc == 0x36) { shift = 1; return; }
    if (sc == 0xAA || sc == 0xB6) { shift = 0; return; }
    if (sc == 0x3A) { caps ^= 1u; return; }
    if (sc & 0x80u) return;

    if (sc < sizeof(normal_map)) {
        char c = (shift || caps) ? shift_map[sc] : normal_map[sc];
        if (shift && caps && sc >= 0x10 && sc <= 0x19) c = normal_map[sc];
        if (c) { push(c); ++keys; }
    }
}

int keyboard_read(char* out) {
    if (tail == head) return 0;
    *out = buffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;
    return 1;
}

uint32_t keyboard_key_count(void) { return keys; }
