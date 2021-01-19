#pragma once

#define FLASH_REGION_SIZE     (512)
#define FLASH_RESERVE_SIZE    (256)

typedef enum
{
    MAIN_AREA,
    RESERVED_AREA
} mock_nvram_area;

typedef enum
{
    ERASE_DISABLED,
    ERASE_ENABLED
} mock_nvram_erase;

void mock_nvram_init(void);

int mock_flash_read(uint8_t * data, uint32_t start, uint32_t length);
int mock_flash_write(uint8_t * data, uint32_t start, uint32_t length);
int mock_flash_erase_main(void);
int mock_flash_erase_reserve(void);

void mock_flash_set(mock_nvram_area area, uint32_t offset, uint8_t value);
void mock_flash_fill_with_random(mock_nvram_area area);
void mock_flash_set_erase(mock_nvram_area area, mock_nvram_erase state);
