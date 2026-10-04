#include "block.h"
#include "ata.h"

int block_init(void){
    return ata_init();
}

int block_present(void){
    return ata_present();
}

int block_read(uint32_t lba, uint8_t *buffer){
    return ata_read28(lba, buffer);
}

int block_write(uint32_t lba, const uint8_t *buffer){
    return ata_write28(lba, buffer);
}

uint32_t block_sector_count(void){
    return ata_sectors();
}

const char *block_model(void){
    return ata_model();
}
