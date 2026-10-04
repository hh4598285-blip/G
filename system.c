#include "keyboard.h"
#include "system.h"
#include "fs.h"
#include "block.h"
#include <stddef.h>

#define VGA ((volatile uint16_t*)0xB8000)
#define W 80
#define H 25
#define CMD_MAX 96

static uint8_t row;
static uint8_t col;
static uint8_t attr = 0x0F;
static uint32_t ticks;
static char command[CMD_MAX];
static uint32_t command_len;

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

static int eq(const char*a,const char*b){
    while(*a&&*b){if(*a++!=*b++)return 0;}
    return *a==*b;
}

static char* skip_spaces(char*p){
    while(*p==' '||*p=='\t')++p;
    return p;
}

static char* next_word(char**cursor){
    char*p=skip_spaces(*cursor);
    char*start=p;
    while(*p&&*p!=' '&&*p!='\t')++p;
    if(*p)*p++=0;
    *cursor=p;
    return start;
}

static void print_uint(uint32_t value){
    char buf[11];
    uint32_t i=0;
    if(!value){system_print("0");return;}
    while(value&&i<10){buf[i++]=(char)('0'+value%10);value/=10;}
    while(i)put(buf[--i]);
}

static void cmd_help(void){
    system_print("Commands:\n");
    system_print(" help  - command list\n");
    system_print(" clear - clear console\n");
    system_print(" ver   - system version\n");
    system_print(" mem   - free heap memory\n");
    system_print(" files - list RAM files\n");
    system_print(" touch NAME - create RAM file\n");
    system_print(" write NAME TEXT - write RAM file\n");
    system_print(" cat NAME - display RAM file\n");
    system_print(" disk - show block device status\n");
}

static void cmd_files(void){
    uint32_t n=fs_count();
    system_print("Files: ");print_uint(n);system_print("\n");
    for(int i=0;i<32;i++){
        const char*name=fs_name(i);
        if(name[0]){
            system_print("  ");system_print(name);system_print("  ");
            print_uint(fs_size(i));system_print(" bytes\n");
        }
    }
}

static void cmd_cat(const char*name){
    int id=fs_find(name);
    if(id<0){system_print("cat: file not found\n");return;}
    char data[4097];
    int n=fs_read(id,data,4096);
    if(n<0){system_print("cat: read error\n");return;}
    data[n]=0;
    system_print(data);
    system_print("\n");
}

static void execute(char*line){
    char*cursor=skip_spaces(line);
    char*cmd=next_word(&cursor);
    if(!cmd[0])return;

    if(eq(cmd,"help")){cmd_help();return;}
    if(eq(cmd,"clear")){clear();return;}
    if(eq(cmd,"ver")||eq(cmd,"version")){system_print("GOS 1.0 - kernel shell\n");return;}
    if(eq(cmd,"mem")){
        extern uint32_t memory_free_bytes(void);
        system_print("Free heap: ");print_uint(memory_free_bytes());system_print(" bytes\n");
        return;
    }
    if(eq(cmd,"files")){cmd_files();return;}
    if(eq(cmd,"disk")){
        if(!block_present()){system_print("BLOCK: no supported disk detected\n");return;}
        system_print("BLOCK: online\n");
        system_print("Model: ");system_print(block_model());system_print("\n");
        system_print("Sectors: ");print_uint(block_sector_count());system_print("\n");
        system_print("Sector size: 512 bytes\n");
        return;
    }
    if(eq(cmd,"touch")){
        char*name=next_word(&cursor);
        if(!name[0]){system_print("touch: missing name\n");return;}
        if(fs_create(name)<0)system_print("touch: cannot create file\n");
        else system_print("file created\n");
        return;
    }
    if(eq(cmd,"write")){
        char*name=next_word(&cursor);
        cursor=skip_spaces(cursor);
        int id=fs_find(name);
        if(id<0)id=fs_create(name);
        if(id<0){system_print("write: cannot create file\n");return;}
        uint32_t n=0;while(cursor[n]&&n<4096)n++;
        if(fs_write(id,cursor,n)<0)system_print("write: write error\n");
        else {system_print("written ");print_uint(n);system_print(" bytes\n");}
        return;
    }
    if(eq(cmd,"cat")){
        char*name=next_word(&cursor);
        if(!name[0]){system_print("cat: missing name\n");return;}
        cmd_cat(name);
        return;
    }
    system_print("Unknown command. Type help.\n");
}

static void prompt(void){
    system_print("GOS> ");
    command_len=0;
    command[0]=0;
}

void system_init(void){
    clear();
    system_print("GOS SYSTEM CONSOLE\n");
    system_print("Kernel services online.\n");
    system_print("Keyboard service online.\n");
    system_print("Filesystem service online.\n");
    system_print("Block device layer online.\n");
    system_print("Type help for commands.\n\n");
    prompt();
}

void system_poll(void){
    ticks++;
    char c;
    if(!keyboard_read(&c))return;

    if(c=='\n'){
        put('\n');
        command[command_len]=0;
        execute(command);
        prompt();
        return;
    }
    if(c=='\b'){
        if(command_len){--command_len;command[command_len]=0;put('\b');}
        return;
    }
    if(c>=32 && c<127 && command_len<CMD_MAX-1){
        command[command_len++]=c;
        command[command_len]=0;
        put(c);
    }
}

uint32_t system_ticks(void){return ticks;}
