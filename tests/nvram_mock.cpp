#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "nvram_mock.h"

static uint8_t flash_memory[FLASH_REGION_SIZE];

void mock_nvram_init(void)
{
	memset(flash_memory, 0xFF, FLASH_REGION_SIZE); 
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
	memset(flash_memory, 0xFF, FLASH_REGION_SIZE - FLASH_RESERVE_SIZE); 

	return 0;
}

int mock_flash_erase_reserve(void)
{
	memset(flash_memory + (FLASH_REGION_SIZE - FLASH_RESERVE_SIZE), 
		   0xFF, FLASH_RESERVE_SIZE); 

	return 0;
}