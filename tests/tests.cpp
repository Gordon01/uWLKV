#include <stdint.h>
#include <string.h>
#include <map>
#include "catch.hpp"
#include "nvram_mock.h"
#include "uwlkv.h"

uwlkv_offset init_uwlkv(uwlkv_offset size, uwlkv_offset reserved)
{
    uwlkv_nvram_interface interface;
    interface.read          = &mock_flash_read;
    interface.write         = &mock_flash_write;
    interface.erase_main    = &mock_flash_erase_main;
    interface.erase_reserve = &mock_flash_erase_reserve;
    /* NVRAM size and reserved space should always match actual sizes of memory
     * which your erase function uses. Here we have an option to override default
     * only for test purposes */
    interface.size 	        = FLASH_REGION_SIZE;
    interface.reserved      = FLASH_RESERVE_SIZE;

    if (size)
    {
        interface.size      = size < FLASH_REGION_SIZE 
                            ? size : FLASH_REGION_SIZE;
    }

    if (reserved)
    {
        interface.reserved  = reserved != FLASH_RESERVE_SIZE 
                            ? reserved :  FLASH_RESERVE_SIZE;
    }
    
    return uwlkv_init(&interface);
}

uwlkv_offset erase_nvram(uwlkv_offset size, uwlkv_offset reserved)
{
    mock_nvram_init();

    return init_uwlkv(size, reserved);
}

TEST_CASE("Initialization", "[init]")
{
    auto ret = erase_nvram(100, 90);
    CHECK(0 == ret);
    ret = init_uwlkv(0, 0);
    CHECK(((FLASH_REGION_SIZE - FLASH_RESERVE_SIZE) / UWLKV_ENTRY_SIZE) == ret);

    auto entries = uwlkv_get_entries_number();
    CHECK(0 == entries);
    auto free = uwlkv_get_free_entries();
    CHECK(UWLKV_MAX_ENTRIES == free);
}

TEST_CASE("Writing and reading values", "[read_write]")
{
    SECTION("Easy values")
    {
        auto test_key   = GENERATE(as<uwlkv_key>{}, 0, 10, 100, 40000);
        auto test_value = GENERATE(100, 1000, 65000, 0);

        auto ret = uwlkv_set_value(test_key, test_value);
        CHECK(UWLKV_E_SUCCESS == ret);

        uwlkv_value read_value;
        ret = uwlkv_get_value(test_key, &read_value);
        CHECK(UWLKV_E_SUCCESS == ret);
        CHECK(read_value == test_value);
    }

    SECTION("Counters")
    {
        auto entries = uwlkv_get_entries_number();
        CHECK(4 == entries);
    }

    SECTION("Using all keys")
    {
        uwlkv_value test_value;
        for(uwlkv_key key = 0; key < UWLKV_MAX_ENTRIES; key++)
        {
            test_value = key + 10000;
            uwlkv_set_value(key, test_value);
        }
        auto free = uwlkv_get_free_entries();
        CHECK(0 == free);

        // Existing key
        auto ret = uwlkv_set_value(1, test_value);
        CHECK(UWLKV_E_SUCCESS == ret);
        // New one
        ret = uwlkv_set_value(UWLKV_MAX_ENTRIES, test_value);
        CHECK(UWLKV_E_NO_SPACE == ret);
    }
}

uint8_t compare_stored_values(std::map<uwlkv_key, uwlkv_value> &map)
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

TEST_CASE("Restarts", "[restarts]")
{
    const auto capacity = erase_nvram(0, 0);
    std::map<uwlkv_key, uwlkv_value> values;

    for(uwlkv_offset i = 0; i < capacity; i++)
    {
        const uwlkv_key   key   = (uwlkv_key)(i % UWLKV_MAX_ENTRIES);
        const uwlkv_value value = (uwlkv_value)i;
        if (UWLKV_E_SUCCESS == uwlkv_set_value(key, value))
        {
            values[key] = value;
        }
    }

    const auto entries = uwlkv_get_entries_number();
    CHECK(entries == values.size());
    init_uwlkv(0, 0);

    // Checking that after restart we have same amount of entries
    CHECK(entries == uwlkv_get_entries_number());
    CHECK(0 == compare_stored_values(values));

    // Add one value to force a rewrite
    uwlkv_set_value(10, 10000);
    values[10] = 10000;
    init_uwlkv(0, 0);
    CHECK(0 == compare_stored_values(values));

    // Finally setting some more values and making sure they present after restart
    uwlkv_set_value(10, 11000);
    uwlkv_set_value(15, 15000);
    values[10] = 11000;
    values[15] = 15000;
    init_uwlkv(0, 0);
    CHECK(0 == compare_stored_values(values));
}