#ifndef GOS_SERVICES_H
#define GOS_SERVICES_H

#include <stdint.h>
#include <stddef.h>

void memory_init(void);
void* gos_malloc(size_t size);
void gos_free(void* p);
uint32_t memory_free_bytes(void);

void fs_init(void);
int fs_create(const char* name);
int fs_write(int id,const void* data,uint32_t size);
int fs_read(int id,void* data,uint32_t size);
int fs_find(const char* name);
uint32_t fs_count(void);

#endif
