/* This module implements a simple cache to track block positions in NVRAM. Key may have 
 * any value within it's type range (uwlkv_key). Currently the cache is implemented as an array. 
 * Due to linear retrieval the access may become slower because of the large amount of unique keys 
 * (defined by UWLKV_MAX_ENTRIES).
 * Also it is stored in RAM so if you want to reduce RAM usage, you may adjust UWLKV_MAX_ENTRIES
 * and data types uwlkv_key and uwlkv_offset. Also you may need to make struct uwlkv_entry packed.
 */

#include "uwlkv.h"
#include "map.h"
#include "entry.h"

static uwlkv_entry           uwlkv_entries[UWLKV_MAX_ENTRIES];
static uwlkv_key             used_entries;

/**
 * @brief	Returns a pointer to an entry with provided key.
 *
 * @param 	   	key  	The key.
 * @param [out]	entry	On success would be pointing to entry in uwlkv_entries.
 *
 * @returns	- UWLKV_E_SUCCESS or
 * 			- UWLKV_E_NOT_EXIST if entry with this key is not found.
 */
uwlkv_error uwlkv_get_entry(const uwlkv_key key, uwlkv_entry ** entry)
{
    for(uwlkv_key i = 0; i < used_entries; i++)
    {
        *entry = &uwlkv_entries[i];
        if (key == uwlkv_entries[i].key)
        {
            return UWLKV_E_SUCCESS;
        }
    }

    return UWLKV_E_NOT_EXIST;
}

uwlkv_entry * uwlkv_get_entry_by_id(const uwlkv_key number)
{
    return &uwlkv_entries[number];
}

/**
 * @brief	Reserves space for one entry and returns a pointer to it. This function does not
 * 			check for free space in uwlkv_entries.
 *
 * @returns	Pointer to an uwlkv_entry.
 */
uwlkv_entry * uwlkv_create_entry(void)
{
    used_entries += 1;

    return &uwlkv_entries[used_entries - 1];
}

/**
 * @brief	Updates the entry information. Creates a new one if entry with provided key currently
 * 			not exist in map.
 *
 * @param 	key   	Entry with this specified key would be modified.
 * @param 	offset	Logical offset of an entry in bytes.
 *
 * @returns	An uwlkv_error.
 */
uwlkv_error uwlkv_update_entry(const uwlkv_key key, const uwlkv_offset offset)
{
    const uwlkv_key free = uwlkv_get_free_space();
    uwlkv_entry * entry;
    if (UWLKV_E_NOT_EXIST == uwlkv_get_entry(key, &entry))
    {
        if (0 == free)
        {
            return UWLKV_E_NO_SPACE;
        }

        entry = uwlkv_create_entry();
        entry->key = key;
    }

    entry->offset = offset;

    return UWLKV_E_SUCCESS;
}

void uwlkv_reset_map(void)
{
    used_entries = 0;
}

/**
 * @brief	Returns a number of stored unique keys.
 *
 * @returns	Number of entries.
 */
uwlkv_key uwlkv_get_used_entries(void)
{
    return used_entries;
}

/**
 * @brief	Returns a number of available unique keys.
 *
 * @returns	Number of entries.
 */
uwlkv_key uwlkv_get_free_space(void)
{
    return UWLKV_MAX_ENTRIES - used_entries;
}
