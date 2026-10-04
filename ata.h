#ifndef GOS_ATA_H
#define GOS_ATA_H
#include <stdint.h>
int ata_init(void);
int ata_present(void);
int ata_read28(uint32_t lba, uint8_t* buffer);
int ata_write28(uint32_t lba, const uint8_t* buffer);
uint32_t ata_sectors(void);
const char* ata_model(void);
#endif
