#pragma once

#define TESTS_STATIC_ENTRIES_NUM  20
#define TESTS_DYNAMIC_ENTRIES_NUM 10

typedef enum
{
    TESTS_NO_CACHE,
    TESTS_STATIC,
    TESTS_DYNAMIC
} tests_cache;

uwlkv_offset tests_init_uwlkv(tests_cache cache_type, uwlkv_offset size, uwlkv_offset reserved);
uwlkv_offset tests_restart_uwlkv(void);
uwlkv_offset tests_erase_nvram(void);
uint8_t tests_compare_stored_values(std::map<uwlkv_key, uwlkv_value> &map);
void tests_fill_main(std::map<uwlkv_key, uwlkv_value> &map, 
                     uwlkv_offset number, uwlkv_value starting_value);
