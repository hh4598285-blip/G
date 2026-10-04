#include "fs.h"
#include <stddef.h>

#define GOS_MAX_FILES 32
#define GOS_NAME 32
#define GOS_DATA 4096

typedef struct {
    char name[GOS_NAME];
    uint32_t size;
    uint8_t used;
    uint8_t data[GOS_DATA];
} gos_file_t;

static gos_file_t files[GOS_MAX_FILES];

static int same(const char*a,const char*b){
    while(*a&&*b){if(*a++!=*b++)return 0;}
    return *a==*b;
}

void fs_init(void){
    for(uint32_t i=0;i<GOS_MAX_FILES;i++){files[i].used=0;files[i].size=0;files[i].name[0]=0;}
}

int fs_create(const char*name){
    if(!name||!name[0])return -1;
    for(uint32_t i=0;i<GOS_MAX_FILES;i++){
        if(files[i].used&&same(files[i].name,name))return -1;
    }
    for(uint32_t i=0;i<GOS_MAX_FILES;i++)if(!files[i].used){
        files[i].used=1;files[i].size=0;
        uint32_t j=0;while(name[j]&&j<GOS_NAME-1){files[i].name[j]=name[j];j++;}files[i].name[j]=0;
        return (int)i;
    }
    return -1;
}

int fs_write(int id,const void*data,uint32_t size){
    if(id<0||id>=GOS_MAX_FILES||!files[id].used||size>GOS_DATA)return -1;
    const uint8_t*s=(const uint8_t*)data;
    for(uint32_t i=0;i<size;i++)files[id].data[i]=s[i];
    files[id].size=size;
    return (int)size;
}

int fs_read(int id,void*data,uint32_t size){
    if(id<0||id>=GOS_MAX_FILES||!files[id].used)return -1;
    if(size>files[id].size)size=files[id].size;
    uint8_t*d=(uint8_t*)data;
    for(uint32_t i=0;i<size;i++)d[i]=files[id].data[i];
    return (int)size;
}

int fs_find(const char*name){
    if(!name)return -1;
    for(uint32_t i=0;i<GOS_MAX_FILES;i++)if(files[i].used&&same(files[i].name,name))return (int)i;
    return -1;
}

uint32_t fs_count(void){
    uint32_t n=0;for(uint32_t i=0;i<GOS_MAX_FILES;i++)if(files[i].used)n++;return n;
}

uint32_t fs_size(int id){
    if(id<0||id>=GOS_MAX_FILES||!files[id].used)return 0;
    return files[id].size;
}

const char* fs_name(int id){
    if(id<0||id>=GOS_MAX_FILES||!files[id].used)return "";
    return files[id].name;
}
