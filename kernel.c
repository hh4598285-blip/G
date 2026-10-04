#include <stdint.h>
#include <stddef.h>

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u
#define MULTIBOOT_INFO_FRAMEBUFFER 0x00001000u

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

#define FB_FLAG 0x00001000u

#define COLOR_RGB(r,g,b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

static volatile uint16_t* vga = VGA_MEMORY;
static uint8_t vga_row = 0;
static uint8_t vga_col = 0;
static uint8_t vga_color = 0x0F;

static volatile uint8_t* framebuffer = (volatile uint8_t*)0;
static uint32_t fb_addr = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t fb_bpp = 0;
static uint8_t fb_type = 0;
static int graphics = 0;

static uint32_t mouse_x = 512;
static uint32_t mouse_y = 384;
static int mouse_ready = 0;
static uint8_t mouse_buttons = 0;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

static void vga_clear(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; ++y) {
        for (uint32_t x = 0; x < VGA_WIDTH; ++x) {
            vga[y * VGA_WIDTH + x] = ((uint16_t)vga_color << 8) | ' ';
        }
    }
    vga_row = 0;
    vga_col = 0;
}

static void vga_newline(void) {
    vga_col = 0;
    if (++vga_row >= VGA_HEIGHT) {
        for (uint32_t y = 1; y < VGA_HEIGHT; ++y) {
            for (uint32_t x = 0; x < VGA_WIDTH; ++x) {
                vga[(y - 1) * VGA_WIDTH + x] = vga[y * VGA_WIDTH + x];
            }
        }
        for (uint32_t x = 0; x < VGA_WIDTH; ++x) {
            vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = ((uint16_t)vga_color << 8) | ' ';
        }
        vga_row = VGA_HEIGHT - 1;
    }
}

static void vga_putc(char c) {
    if (c == '\n') {
        vga_newline();
        return;
    }
    if (c == '\r') {
        vga_col = 0;
        return;
    }
    if (c == '\b') {
        if (vga_col > 0) {
            --vga_col;
            vga[vga_row * VGA_WIDTH + vga_col] = ((uint16_t)vga_color << 8) | ' ';
        }
        return;
    }
    if (vga_col >= VGA_WIDTH) {
        vga_newline();
    }
    vga[vga_row * VGA_WIDTH + vga_col] = ((uint16_t)vga_color << 8) | (uint8_t)c;
    ++vga_col;
}

static void vga_write(const char* text) {
    while (*text) {
        vga_putc(*text++);
    }
}

static void vga_hex(uint32_t value) {
    const char* digits = "0123456789ABCDEF";
    vga_write("0x");
    for (int i = 7; i >= 0; --i) {
        vga_putc(digits[(value >> (i * 4)) & 0xF]);
    }
}

static uint32_t abs_u32(int32_t value) {
    return value < 0 ? (uint32_t)(-value) : (uint32_t)value;
}

static void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!graphics || x >= fb_width || y >= fb_height) {
        return;
    }
    volatile uint8_t* p = framebuffer + y * fb_pitch + x * (fb_bpp / 8);
    if (fb_bpp == 32) {
        *(volatile uint32_t*)p = color;
    } else if (fb_bpp == 24) {
        p[0] = (uint8_t)(color & 0xFF);
        p[1] = (uint8_t)((color >> 8) & 0xFF);
        p[2] = (uint8_t)((color >> 16) & 0xFF);
    } else if (fb_bpp == 16) {
        uint16_t r = (uint16_t)((color >> 19) & 0x1F);
        uint16_t g = (uint16_t)((color >> 10) & 0x3F);
        uint16_t b = (uint16_t)((color >> 3) & 0x1F);
        *(volatile uint16_t*)p = (uint16_t)((r << 11) | (g << 5) | b);
    }
}

static void fb_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if (!graphics || w <= 0 || h <= 0) {
        return;
    }
    int32_t x0 = x < 0 ? 0 : x;
    int32_t y0 = y < 0 ? 0 : y;
    int32_t x1 = x + w;
    int32_t y1 = y + h;
    if (x1 > (int32_t)fb_width) x1 = (int32_t)fb_width;
    if (y1 > (int32_t)fb_height) y1 = (int32_t)fb_height;
    for (int32_t yy = y0; yy < y1; ++yy) {
        for (int32_t xx = x0; xx < x1; ++xx) {
            fb_put_pixel((uint32_t)xx, (uint32_t)yy, color);
        }
    }
}

static void fb_border(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    fb_fill_rect(x, y, w, 2, color);
    fb_fill_rect(x, y + h - 2, w, 2, color);
    fb_fill_rect(x, y, 2, h, color);
    fb_fill_rect(x + w - 2, y, 2, h, color);
}

static void fb_draw_char(int32_t x, int32_t y, char c, uint32_t color, uint32_t scale) {
    static const uint8_t font[16][8] = {
        {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
        {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
        {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},
        {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
        {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00},
        {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
        {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},
        {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
        {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
        {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},
        {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x7C,0x66,0x66,0x7C,0x66,0x66,0x66,0x00},
        {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
        {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
        {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}
    };
    uint8_t index;
    if (c >= '0' && c <= '9') index = (uint8_t)(c - '0');
    else if (c == ':') index = 10;
    else if (c == ' ') index = 11;
    else if (c >= 'A' && c <= 'D') index = (uint8_t)(12 + c - 'A');
    else index = 11;
    for (uint32_t row = 0; row < 8; ++row) {
        for (uint32_t col = 0; col < 8; ++col) {
            if (font[index][row] & (uint8_t)(1u << (7 - col))) {
                fb_fill_rect(x + (int32_t)(col * scale), y + (int32_t)(row * scale), (int32_t)scale, (int32_t)scale, color);
            }
        }
    }
}

static void fb_text(int32_t x, int32_t y, const char* text, uint32_t color, uint32_t scale) {
    int32_t start_x = x;
    while (*text) {
        if (*text == '\n') {
            y += (int32_t)(10 * scale);
            x = start_x;
        } else {
            fb_draw_char(x, y, *text, color, scale);
            x += (int32_t)(9 * scale);
        }
        ++text;
    }
}

static void draw_window(int32_t x, int32_t y, int32_t w, int32_t h, const char* title, uint32_t accent) {
    fb_fill_rect(x + 5, y + 6, w, h, COLOR_RGB(8, 10, 18));
    fb_border(x, y, w, h, COLOR_RGB(75, 82, 105));
    fb_fill_rect(x + 2, y + 2, w - 4, 28, accent);
    fb_fill_rect(x + 2, y + 30, w - 4, h - 32, COLOR_RGB(25, 29, 43));
    fb_text(x + 12, y + 9, title, COLOR_RGB(255,255,255), 2);
    fb_fill_rect(x + w - 64, y + 10, 8, 8, COLOR_RGB(255,210,80));
    fb_fill_rect(x + w - 45, y + 10, 8, 8, COLOR_RGB(90,220,130));
    fb_fill_rect(x + w - 26, y + 10, 8, 8, COLOR_RGB(240,90,100));
}

static void draw_cursor(void) {
    static const uint8_t shape[16] = {
        0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0xFE,0xFF,
        0xFE,0xFC,0xF8,0xF0,0xD8,0x18,0x0C,0x00
    };
    for (uint32_t row = 0; row < 16; ++row) {
        for (uint32_t col = 0; col < 8; ++col) {
            if (shape[row] & (uint8_t)(1u << (7 - col))) {
                fb_put_pixel(mouse_x + col, mouse_y + row, COLOR_RGB(255,255,255));
            }
        }
    }
}

static void draw_desktop(void) {
    fb_fill_rect(0, 0, (int32_t)fb_width, (int32_t)fb_height, COLOR_RGB(12, 16, 28));
    for (uint32_t y = 0; y < fb_height; y += 4) {
        uint8_t shade = (uint8_t)(18 + (y * 12 / (fb_height ? fb_height : 1)));
        fb_fill_rect(0, (int32_t)y, (int32_t)fb_width, 4, COLOR_RGB(shade, shade + 3, shade + 10));
    }
    fb_fill_rect(0, 0, (int32_t)fb_width, 42, COLOR_RGB(20, 24, 38));
    fb_text(18, 10, "GOS", COLOR_RGB(110, 190, 255), 2);
    fb_text(80, 10, "DESKTOP", COLOR_RGB(235,240,250), 2);

    draw_window(45, 80, 470, 290, "SYSTEM", COLOR_RGB(42, 96, 170));
    fb_text(72, 135, "GOS", COLOR_RGB(255,255,255), 3);
    fb_text(72, 175, "READY", COLOR_RGB(120,230,170), 2);
    fb_text(72, 210, "FRAMEBUFFER", COLOR_RGB(190,200,220), 2);
    fb_text(72, 240, "MULTIBOOT", COLOR_RGB(190,200,220), 2);
    fb_text(72, 270, "KERNEL ONLINE", COLOR_RGB(190,200,220), 2);

    draw_window(555, 80, 420, 220, "CONTROL", COLOR_RGB(100, 70, 170));
    fb_text(580, 135, "CPU", COLOR_RGB(220,225,240), 2);
    fb_fill_rect(580, 162, 330, 14, COLOR_RGB(45,50,68));
    fb_fill_rect(580, 162, 135, 14, COLOR_RGB(90,180,245));
    fb_text(580, 195, "MEMORY", COLOR_RGB(220,225,240), 2);
    fb_fill_rect(580, 222, 330, 14, COLOR_RGB(45,50,68));
    fb_fill_rect(580, 222, 205, 14, COLOR_RGB(120,220,160));

    fb_fill_rect(0, (int32_t)fb_height - 48, (int32_t)fb_width, 48, COLOR_RGB(20,24,38));
    fb_text(20, (int32_t)fb_height - 36, "MENU", COLOR_RGB(255,255,255), 2);
    fb_text((int32_t)fb_width - 180, (int32_t)fb_height - 36, "GOS 1.0", COLOR_RGB(170,185,210), 2);
}

static int ps2_wait_input(void) {
    for (uint32_t i = 0; i < 100000; ++i) {
        if (inb(0x64) & 1u) return 1;
    }
    return 0;
}

static int ps2_wait_write(void) {
    for (uint32_t i = 0; i < 100000; ++i) {
        if (!(inb(0x64) & 2u)) return 1;
    }
    return 0;
}

static int ps2_mouse_write(uint8_t value) {
    if (!ps2_wait_write()) return 0;
    outb(0x64, 0xD4);
    if (!ps2_wait_write()) return 0;
    outb(0x60, value);
    return 1;
}

static void mouse_init(void) {
    if (!ps2_wait_write()) return;
    outb(0x64, 0xA8);
    io_wait();
    if (!ps2_wait_write()) return;
    outb(0x64, 0x20);
    if (!ps2_wait_input()) return;
    uint8_t status = inb(0x60);
    status |= 2u;
    status &= (uint8_t)~0x20u;
    if (!ps2_wait_write()) return;
    outb(0x64, 0x60);
    if (!ps2_wait_write()) return;
    outb(0x60, status);
    if (!ps2_mouse_write(0xF6)) return;
    if (!ps2_wait_input()) return;
    (void)inb(0x60);
    if (!ps2_mouse_write(0xF4)) return;
    if (!ps2_wait_input()) return;
    (void)inb(0x60);
    mouse_ready = 1;
}

static int mouse_read_byte(uint8_t* value) {
    if (!(inb(0x64) & 1u)) return 0;
    uint8_t status = inb(0x64);
    if (!(status & 0x20u)) return 0;
    *value = inb(0x60);
    return 1;
}

static void mouse_poll(void) {
    static uint8_t packet[3];
    static uint8_t index = 0;
    uint8_t value;
    while (mouse_ready && mouse_read_byte(&value)) {
        if (index == 0 && !(value & 0x08u)) continue;
        packet[index++] = value;
        if (index == 3) {
            index = 0;
            mouse_buttons = packet[0] & 7u;
            int32_t dx = (int8_t)packet[1];
            int32_t dy = (int8_t)packet[2];
            if (packet[0] & 0x40u) dx = 0;
            if (packet[0] & 0x80u) dy = 0;
            if (dx < 0 && mouse_x < abs_u32(dx)) mouse_x = 0;
            else if (dx > 0 && mouse_x + (uint32_t)dx < fb_width) mouse_x += (uint32_t)dx;
            else if (dx > 0) mouse_x = fb_width - 1;
            if (dy > 0 && mouse_y < (uint32_t)dy) mouse_y = 0;
            else if (dy < 0 && mouse_y + (uint32_t)(-dy) < fb_height) mouse_y += (uint32_t)(-dy);
            else if (dy < 0) mouse_y = fb_height - 1;
            if (mouse_x >= fb_width) mouse_x = fb_width - 1;
            if (mouse_y >= fb_height) mouse_y = fb_height - 1;
            draw_desktop();
            draw_cursor();
        }
    }
}

static void init_framebuffer(uint32_t magic, uint32_t mbi_addr) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || mbi_addr == 0) return;
    volatile uint32_t* info = (volatile uint32_t*)(uintptr_t)mbi_addr;
    uint32_t flags = info[0];
    if (!(flags & MULTIBOOT_INFO_FRAMEBUFFER)) return;
    fb_addr = info[22];
    fb_pitch = info[24];
    fb_width = info[25];
    fb_height = info[26];
    fb_bpp = *(volatile uint8_t*)((uintptr_t)mbi_addr + 108);
    fb_type = *(volatile uint8_t*)((uintptr_t)mbi_addr + 109);
    if (fb_addr == 0 || fb_width < 640 || fb_height < 480 || fb_bpp < 16 || fb_bpp > 32) return;
    if (fb_type != 1 && fb_type != 2) return;
    framebuffer = (volatile uint8_t*)(uintptr_t)fb_addr;
    graphics = 1;
}

void kmain(uint32_t magic, uint32_t mbi_addr) {
    vga_clear();
    vga_write("GOS kernel starting...\n");
    vga_write("Multiboot magic: ");
    vga_hex(magic);
    vga_write("\n");

    init_framebuffer(magic, mbi_addr);

    if (!graphics) {
        vga_write("Framebuffer unavailable; using VGA text mode.\n");
        vga_write("System online.\n");
        for (;;) {
            __asm__ volatile("hlt");
        }
    }

    draw_desktop();
    mouse_x = fb_width / 2;
    mouse_y = fb_height / 2;
    mouse_init();
    draw_cursor();

    for (;;) {
        mouse_poll();
        __asm__ volatile("hlt");
    }
}
