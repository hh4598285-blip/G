#ifndef GOS_FS_H
#define GOS_FS_H
#include <stdint.h>
int fs_create(const char* name);
int fs_write(int id,const void* data,uint32_t size);
int fs_read(int id,void* data,uint32_t size);
int fs_find(const char* name);
uint32_t fs_count(void);
uint32_t fs_size(int id);
const char* fs_name(int id);
#endif
