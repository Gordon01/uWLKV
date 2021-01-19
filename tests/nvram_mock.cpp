#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "nvram_mock.h"

static uint8_t flash_memory[FLASH_REGION_SIZE];
static mock_nvram_erase main_erase_status, reserve_erase_status;

void mock_nvram_init(void)
{
	memset(flash_memory, 0xFF, FLASH_REGION_SIZE); 

	main_erase_status = ERASE_ENABLED;
	reserve_erase_status = ERASE_ENABLED;
}

int mock_flash_read(uint8_t * data, uint32_t start, uint32_t length)
{
	if ((start + length) > FLASH_REGION_SIZE)
	{
		return 1;
	}

	memcpy(data, flash_memory + start, length);
	return 0;
}

int mock_flash_write(uint8_t * data, uint32_t start, uint32_t length)
{
	if ((start + length) > FLASH_REGION_SIZE)
	{
		return 1;
	}

	/* Real flash memory should be erased before writing. To simulate this,
	 * we temporarily read a requested block and check that it filled with 0xFF */
	uint8_t * tmp_data = (uint8_t *)alloca(length);
	mock_flash_read(tmp_data, start, length);
	for (uint32_t i = 0; i < length; i++)
	{
		if (tmp_data[i] != 0xFF) 
		{
			return 2;
		}
	}

	memcpy(flash_memory + start, data, length);
	return 0;
}

int mock_flash_erase_main(void)
{
	if (ERASE_ENABLED == main_erase_status)
	{
		memset(flash_memory, 0xFF, FLASH_REGION_SIZE - FLASH_RESERVE_SIZE); 
	}

	return 0;
}

int mock_flash_erase_reserve(void)
{
	if (ERASE_ENABLED == reserve_erase_status)
	{
		memset(flash_memory + (FLASH_REGION_SIZE - FLASH_RESERVE_SIZE), 
			0xFF, FLASH_RESERVE_SIZE); 
	}

	return 0;
}

void mock_flash_set(mock_nvram_area area, uint32_t offset, uint8_t value)
{
	if (RESERVED_AREA == area)
	{
		offset += FLASH_REGION_SIZE - FLASH_RESERVE_SIZE;
	}

	flash_memory[offset] = value;
}

void mock_flash_fill_with_random(mock_nvram_area area)
{
	uint32_t offset;
	uint32_t end;

	if (MAIN_AREA == area)
	{
		offset = 0;
		end = FLASH_REGION_SIZE - FLASH_RESERVE_SIZE;
	}
	else
	{
		offset = FLASH_REGION_SIZE - FLASH_RESERVE_SIZE;
		end = FLASH_REGION_SIZE;
	}
	
	for(; offset < end; offset++)
	{
		flash_memory[offset] = (uint8_t)(rand() % 255);
	}
}

void mock_flash_set_erase(mock_nvram_area area, mock_nvram_erase state)
{
	if (MAIN_AREA == area)
	{
		main_erase_status = state;
	}
	else
	{
		reserve_erase_status = state;
	}
}