#ifndef GOS_BLOCK_H
#define GOS_BLOCK_H
#include <stdint.h>

#define GOS_BLOCK_SIZE 512u

int block_init(void);
int block_present(void);
int block_read(uint32_t lba, uint8_t *buffer);
int block_write(uint32_t lba, const uint8_t *buffer);
uint32_t block_sector_count(void);
const char *block_model(void);

#endif
