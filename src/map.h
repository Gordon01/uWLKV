#ifndef UWLKV_MAP_H
#define UWLKV_MAP_H

uwlkv_error uwlkv_get_entry(const uwlkv_key key, uwlkv_entry ** entry);
uwlkv_entry * uwlkv_get_entry_by_id(const uwlkv_key number);
uwlkv_entry * uwlkv_create_entry(void);
uwlkv_error uwlkv_update_entry(const uwlkv_key key, const uwlkv_offset offset);
void uwlkv_reset_map(void);
uwlkv_key uwlkv_get_used_entries(void);
uwlkv_key uwlkv_map_free_entries(void);

#endif
