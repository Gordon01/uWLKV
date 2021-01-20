#include <map>
#include <stdint.h>
#include "uwlkv.h"
#include "nvram_mock.h"
#include "helpers.h"

uwlkv_offset tests_init_uwlkv(const uwlkv_offset size, const uwlkv_offset reserved,
                              const uwlkv_key capacity, uwlkv_double_capacity double_capacity)
{
    uwlkv_nvram_interface nvram_interface;
    nvram_interface.read          = &mock_flash_read;
    nvram_interface.write         = &mock_flash_write;
    nvram_interface.erase_main    = &mock_flash_erase_main;
    nvram_interface.erase_reserve = &mock_flash_erase_reserve;
    /* NVRAM size and reserved space should always match actual sizes of memory
     * which your erase function uses. Here we have an option to override default
     * only for test purposes */
    nvram_interface.size 	      = FLASH_REGION_SIZE;
    nvram_interface.reserved      = FLASH_RESERVE_SIZE;

    if (size)
    {
        nvram_interface.size      = size < FLASH_REGION_SIZE 
                                  ? size : FLASH_REGION_SIZE;
    }

    if (reserved)
    {
        nvram_interface.reserved  = reserved != FLASH_RESERVE_SIZE 
                                  ? reserved :  FLASH_RESERVE_SIZE;
    }
    
    uwlkv_cache_interface cache_interface;
    cache_interface.capacity        = capacity;
    cache_interface.double_capacity = double_capacity;

    return uwlkv_init(&nvram_interface, &cache_interface);
}

uwlkv_offset tests_erase_nvram(uwlkv_offset size, uwlkv_offset reserved)
{
    mock_nvram_init();

    return tests_init_uwlkv(size, reserved, UWLKV_MAX_ENTRIES, nullptr);
}

uint8_t tests_compare_stored_values(std::map<uwlkv_key, uwlkv_value> &map)
{
    auto errors = 0;
    for (auto const& entry : map)
    {
        uwlkv_value value;
        uwlkv_get_value(entry.first, &value);
        if (value != entry.second)
        {
            errors += 1;
        }
    }

    return (errors != 0);
}

void tests_fill_main(std::map<uwlkv_key, uwlkv_value> &map, 
                     uwlkv_offset number, uwlkv_value starting_value)
{
    for(uwlkv_offset i = 0; i < number; i++)
    {
        const uwlkv_key   key   = (uwlkv_key)(i % UWLKV_MAX_ENTRIES);
        const uwlkv_value value = (uwlkv_value)i + starting_value;
        if (UWLKV_E_SUCCESS == uwlkv_set_value(key, value))
        {
            map[key] = value;
        }
    }
}
