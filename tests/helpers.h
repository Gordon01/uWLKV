#pragma once

uwlkv_offset tests_init_uwlkv(const uwlkv_offset size, const uwlkv_offset reserved,
                              const uwlkv_key capacity, uwlkv_double_capacity double_capacity);
uwlkv_offset tests_erase_nvram(uwlkv_offset size, uwlkv_offset reserved);
uint8_t tests_compare_stored_values(std::map<uwlkv_key, uwlkv_value> &map);
void tests_fill_main(std::map<uwlkv_key, uwlkv_value> &map, 
                     uwlkv_offset number, uwlkv_value starting_value);
