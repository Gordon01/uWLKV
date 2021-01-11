#ifndef UWLKV_ENTRY_H
#define UWLKV_ENTRY_H

uwlkv_error uwlkv_read_entry(uwlkv_offset offset, uwlkv_key * key, uwlkv_value * value);
uwlkv_error uwlkv_write_entry(uwlkv_offset offset, uwlkv_key key, uwlkv_value value);
uint8_t uwlkv_is_block_erased(const uint8_t * data, const uwlkv_offset size);

#endif
