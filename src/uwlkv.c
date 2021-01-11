#include "uwlkv.h"
#include "entry.h"
#include "map.h"
#include "storage.h"

uwlkv_nvram_interface nvram_interface;
uint8_t uwlkv_initialized = 0;

/**
 * @brief	Reads NVRAM content and builds its map
 *
 * @param [in]	interface	NVRAM access insterface.
 *
 * @returns	- NVRAM capacity in entries. This value, divided by UWLKV_MAX_ENTRIES gives you an
 * 			expected leveling factor or write cycles multiplier.
 * 			- 0 if NVRAM size is too small to fit all entries.
 */
uwlkv_offset uwlkv_init(const uwlkv_nvram_interface * interface)
{
    const uwlkv_offset main_size        = interface->size - interface->reserved;
    const uwlkv_offset reserve_capacity = interface->reserved / UWLKV_ENTRY_SIZE;
    const uwlkv_offset main_capacity    = main_size / UWLKV_ENTRY_SIZE;

    const uint8_t reserve_size_wrong   = interface->reserved >= interface->size;
    const uint8_t main_smaller_reserve = main_capacity < reserve_capacity;

    if (    (reserve_size_wrong)
        ||  (main_smaller_reserve)
        ||  (main_capacity    <= UWLKV_MAX_ENTRIES)
        ||  (reserve_capacity <= UWLKV_MAX_ENTRIES) )
    {
        return 0;
    }

    nvram_interface = *interface;

    uwlkv_cold_boot();

    uwlkv_initialized = 1;

    return main_capacity;
}

/**
 * @brief	Get value of specifiend key
 *
 * @param 	   	key  	The key.
 * @param [out]	value	Read value if success.
 *
 * @returns	UWLKV_E_SUCCESS on sucesseful read.
 */
uwlkv_error uwlkv_get_value(uwlkv_key key, uwlkv_value * value)
{
    if (0 == uwlkv_initialized)
    {
        return UWLKV_E_NOT_STARTED;
    }

    uwlkv_entry * entry;
    if (uwlkv_get_entry(key, &entry))
    {
        return UWLKV_E_NOT_EXIST;
    }

    return uwlkv_read_entry(entry->offset, &key, value);
}

/**
 * @brief	Set value of specified key.
 *
 * @param 	key  	The key.
 * @param 	value	Value to be written.
 *
 * @returns	UWLKV_E_SUCCESS on sucesseful read.
 */
uwlkv_error uwlkv_set_value(uwlkv_key key, uwlkv_value value)
{
    if (0 == uwlkv_initialized)
    {
        return UWLKV_E_NOT_STARTED;
    }

    uwlkv_offset offset = uwlkv_get_next_block();
    uwlkv_offset last_offset;
    uwlkv_entry * entry;
    const uint8_t new_entry = UWLKV_E_SUCCESS == uwlkv_get_entry(key, &entry) ? 0 : 1;

    if (0 == new_entry)
    {
        // If entry already exists, save its offset in case of write error
        last_offset = entry->offset;
    }

    uwlkv_error update = uwlkv_update_entry(key, offset);
    if (UWLKV_E_SUCCESS != update)
    {
        return update;
    }

    uwlkv_error write = uwlkv_write_entry(offset, key, value);
    if (    (UWLKV_E_SUCCESS != write)
        &&  (new_entry) )
    {
        // On failed write, restore original entry offset
        uwlkv_update_entry(key, last_offset);
    }

    return write;
}

/**
 * @brief	Returns number of unique key values in use.
 *
 * @returns	Number of keys.
 */
uwlkv_key uwlkv_get_entries_number(void)
{
    return uwlkv_get_used_entries();
}

/**
 * @brief	Returns number of free unique key values.
 *
 * @returns	Number of keys.
 */
uwlkv_key uwlkv_get_free_entries(void)
{
    return uwlkv_get_free_space();
}
