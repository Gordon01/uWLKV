#include <map>
#include <stdint.h>
#include <string.h>
#include "catch.hpp"
#include "nvram_mock.h"
#include "uwlkv.h"
#include "helpers.h"

TEST_CASE("Initialization", "[init]")
{
    mock_nvram_init();
    auto ret = tests_init_uwlkv(TESTS_STATIC, 100, 90);
    CHECK(0 == ret);
    ret = tests_init_uwlkv(TESTS_STATIC, 0, 0);
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

TEST_CASE("Data wraps", "[wraps]")
{
    const auto capacity = tests_erase_nvram();
    std::map<uwlkv_key, uwlkv_value> values;

    // Prepare a state where main area is fully filled
    tests_fill_main(values, capacity, 0);

    const auto entries = uwlkv_get_entries_number();
    CHECK(entries == values.size());
    CHECK(entries == UWLKV_MAX_ENTRIES);

    SECTION("No wrap")
    {
        tests_restart_uwlkv();
        CHECK(0 == tests_compare_stored_values(values));
    }

    SECTION("Basic wrap")
    {
        // Add one value to force a rewrite
        uwlkv_set_value(10, 10000);
        values[10] = 10000;
        tests_restart_uwlkv();
        CHECK(0 == tests_compare_stored_values(values));
    }

    SECTION("Interrupted erase of main area")
    {
        mock_flash_set_erase(RESERVED_AREA, ERASE_DISABLED);
        uwlkv_set_value(10, 10000);

        mock_flash_set_erase(RESERVED_AREA, ERASE_ENABLED);
        mock_flash_fill_with_random(MAIN_AREA);
        mock_flash_set(RESERVED_AREA, UWLKV_O_ERASE_STARTED,  UWLKV_NVRAM_ERASE_STARTED);
        mock_flash_set(RESERVED_AREA, UWLKV_O_ERASE_FINISHED, UWLKV_ERASED_BYTE_VALUE);

        tests_restart_uwlkv();
        CHECK(0 == tests_compare_stored_values(values));
    }

    SECTION("Interrupted erase of reserved area")
    {
        mock_flash_fill_with_random(RESERVED_AREA);
        mock_flash_set(MAIN_AREA, UWLKV_O_ERASE_STARTED,  UWLKV_NVRAM_ERASE_STARTED);
        mock_flash_set(MAIN_AREA, UWLKV_O_ERASE_FINISHED, UWLKV_ERASED_BYTE_VALUE);

        tests_restart_uwlkv();
        CHECK(0 == tests_compare_stored_values(values));
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

        tests_restart_uwlkv();
        CHECK(0 == tests_compare_stored_values(values));
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

        tests_restart_uwlkv();
        CHECK(0 == tests_compare_stored_values(values));
    }

    // Finally making sure that library works normally after recovery
    tests_fill_main(values, UWLKV_MAX_ENTRIES, 100);
    CHECK(0 == tests_compare_stored_values(values));
 
    tests_restart_uwlkv();
    CHECK(0 == tests_compare_stored_values(values));
    tests_fill_main(values, (capacity * 2), 10000);
    CHECK(0 == tests_compare_stored_values(values));
    tests_restart_uwlkv();
    CHECK(0 == tests_compare_stored_values(values));
}