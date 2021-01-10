#ifndef UWLKV_ENTRY_H
#define UWLKV_ENTRY_H

uwlkv_error uwlkv_read_entry(uwlkv_offset offset, uwlkv_key * key, uwlkv_value * value);
uwlkv_error uwlkv_write_entry(uwlkv_offset offset, uwlkv_key key, uwlkv_value value);

#endif
