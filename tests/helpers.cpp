#include <map>
#include <stdint.h>
#include "uwlkv.h"
#include "nvram_mock.h"
#include "helpers.h"

static tests_cache tests_cache_type;
static uwlkv_entry tests_static_entries[TESTS_STATIC_ENTRIES_NUM];
static uwlkv_entry tests_dynamic_entries[TESTS_DYNAMIC_ENTRIES_NUM];

uwlkv_offset tests_init_uwlkv(const uwlkv_offset size, const uwlkv_offset reserved, uint8_t * cache,
                              const uwlkv_key capacity, uwlkv_double_capacity double_capacity)
{
    uwlkv_nvram_interface nvram_interface;
    nvram_interface.read          = &mock_flash_read;
    nvram_interface.write         = &mock_flash_write;
    nvram_interface.erase_main    = &mock_flash_erase_main;
    nvram_interface.erase_reserve = &mock_flash_erase_reserve;
    /* The size and reserved space must match the memory, which can be erased by the erase functions.
     * Here we have an option to override a default for testing purposes only. */
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
    cache_interface.cache           = cache;
    cache_interface.capacity        = capacity;
    cache_interface.double_capacity = double_capacity;

    return uwlkv_init(&nvram_interface, &cache_interface);
}

uint8_t * tests_double_capacity(void)
{
    return nullptr;
}

uwlkv_offset tests_init_uwlkv(tests_cache cache_type, uwlkv_offset size, uwlkv_offset reserved)
{
    tests_cache_type = cache_type;

    switch (cache_type)
    {
    case TESTS_NO_CACHE:
        return tests_init_uwlkv(size, reserved, nullptr, 0, nullptr);
    
    case TESTS_STATIC:
        return tests_init_uwlkv(size, reserved, (uint8_t *)&tests_static_entries, 
                                TESTS_STATIC_ENTRIES_NUM, nullptr);

    case TESTS_DYNAMIC:
        return tests_init_uwlkv(size, reserved, (uint8_t *)&tests_dynamic_entries, 
                                TESTS_DYNAMIC_ENTRIES_NUM, &tests_double_capacity);
    }

    return 0;
}

uwlkv_offset tests_restart_uwlkv(void)
{
    return tests_init_uwlkv(tests_cache_type, 0, 0);
}

uwlkv_offset tests_erase_nvram(void)
{
    mock_nvram_init();

    return tests_init_uwlkv(tests_cache_type, 0, 0);
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
