#pragma once

#define FLASH_REGION_SIZE     (512)
#define FLASH_RESERVE_SIZE    (256)

void mock_nvram_init(void);

int mock_flash_read(uint8_t * data, uint32_t start, uint32_t length);
int mock_flash_write(uint8_t * data, uint32_t start, uint32_t length);
int mock_flash_erase_main(void);
int mock_flash_erase_reserve(void);