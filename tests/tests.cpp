#include <map>
#include <ostream>
#include <stdint.h>
#include <string.h>

#include "nvram_mock.h"
#include "uwlkv.h"

inline std::ostream &operator<<(std::ostream &os, uwlkv_error e)
{
    switch (e)
    {
    case UWLKV_E_SUCCESS:
        return os << "OK";
    case UWLKV_E_NOT_EXIST:
        return os << "Requested entry does not exist on NVRAM";
    case UWLKV_E_NVRAM_ERROR:
        return os << "NVRAM interface signalled an error during the operation";
    case UWLKV_E_NOT_STARTED:
        return os << "UWLKV haven't been initialized";
    case UWLKV_E_NO_SPACE:
        return os << "No free space in map for new entry";
    case UWLKV_E_WRONG_OFFSET:
        return os << "Provided offset is out of NVRAM bounds";
    default:
        return os << "uwlkv_error(" << e << ")";
    }
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

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
        uwlkv_value test_value = 0;
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

void fill_main(std::map<uwlkv_key, uwlkv_value> &map, uwlkv_offset number, uwlkv_value starting_value)
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

TEST_CASE("Data wraps", "[wraps]")
{
    const auto capacity = erase_nvram(0, 0);
    std::map<uwlkv_key, uwlkv_value> values;

    // Prepare a state where main area is fully filled
    fill_main(values, capacity, 0);

    const auto entries = uwlkv_get_entries_number();
    CHECK(entries == values.size());
    CHECK(entries == UWLKV_MAX_ENTRIES);

    SECTION("No wrap")
    {
        init_uwlkv(0, 0);
        CHECK(0 == compare_stored_values(values));
    }

    SECTION("Basic wrap")
    {
        // Add one value to force a rewrite
        uwlkv_set_value(10, 10000);
        values[10] = 10000;
        init_uwlkv(0, 0);
        CHECK(0 == compare_stored_values(values));
    }

    SECTION("Interrupted erase of main area")
    {
        mock_flash_set_erase(RESERVED_AREA, ERASE_DISABLED);
        uwlkv_set_value(10, 10000);

        mock_flash_set_erase(RESERVED_AREA, ERASE_ENABLED);
        mock_flash_fill_with_random(MAIN_AREA);
        mock_flash_set(RESERVED_AREA, UWLKV_O_ERASE_STARTED,  UWLKV_NVRAM_ERASE_STARTED);
        mock_flash_set(RESERVED_AREA, UWLKV_O_ERASE_FINISHED, UWLKV_ERASED_BYTE_VALUE);

        init_uwlkv(0, 0);
        CHECK(0 == compare_stored_values(values));
    }

    SECTION("Interrupted erase of reserved area")
    {
        mock_flash_fill_with_random(RESERVED_AREA);
        mock_flash_set(MAIN_AREA, UWLKV_O_ERASE_STARTED,  UWLKV_NVRAM_ERASE_STARTED);
        mock_flash_set(MAIN_AREA, UWLKV_O_ERASE_FINISHED, UWLKV_ERASED_BYTE_VALUE);

        init_uwlkv(0, 0);
        CHECK(0 == compare_stored_values(values));
    }

    SECTION("Interrupted transfer from main to reserve")
    {
        /* Power loss is simulated by filling reserved area with random data but not 
         * setting flags MAIN_ERASE_STARTED and MAIN_ERASE_FINISHED. Therefore, library 
         * should discard data in reserved area and erase it */
        mock_flash_fill_with_random(RESERVED_AREA);
        mock_flash_set(MAIN_AREA, UWLKV_O_ERASE_STARTED,      UWLKV_NVRAM_ERASE_STARTED);
        mock_flash_set(MAIN_AREA, UWLKV_O_ERASE_FINISHED,     UWLKV_NVRAM_ERASE_FINISHED);
        mock_flash_set(RESERVED_AREA, UWLKV_O_ERASE_STARTED,  UWLKV_ERASED_BYTE_VALUE);
        mock_flash_set(RESERVED_AREA, UWLKV_O_ERASE_FINISHED, UWLKV_ERASED_BYTE_VALUE);

        init_uwlkv(0, 0);
        CHECK(0 == compare_stored_values(values));
    }

    SECTION("Interrupted tranfer from reserve to main")
    {
        mock_flash_set_erase(RESERVED_AREA, ERASE_DISABLED);
        uwlkv_set_value(10, 10000);

        mock_flash_set_erase(RESERVED_AREA, ERASE_ENABLED);
        mock_flash_fill_with_random(MAIN_AREA);
        mock_flash_set(MAIN_AREA,     UWLKV_O_ERASE_STARTED,  UWLKV_ERASED_BYTE_VALUE);
        mock_flash_set(MAIN_AREA,     UWLKV_O_ERASE_FINISHED, UWLKV_ERASED_BYTE_VALUE);
        mock_flash_set(RESERVED_AREA, UWLKV_O_ERASE_STARTED,  UWLKV_NVRAM_ERASE_STARTED);
        mock_flash_set(RESERVED_AREA, UWLKV_O_ERASE_FINISHED, UWLKV_NVRAM_ERASE_FINISHED);

        init_uwlkv(0, 0);
        CHECK(0 == compare_stored_values(values));
    }

    // Finally making sure that library works normally after recovery
    fill_main(values, UWLKV_MAX_ENTRIES, 100);
    CHECK(0 == compare_stored_values(values));
 
    init_uwlkv(0, 0);
    CHECK(0 == compare_stored_values(values));
    fill_main(values, (capacity * 2), 10000);
    CHECK(0 == compare_stored_values(values));
    init_uwlkv(0, 0);
    CHECK(0 == compare_stored_values(values));
}