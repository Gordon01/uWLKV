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

extern uwlkv_nvram_interface nvram_interface;
static uwlkv_entry           uwlkv_entries[UWLKV_MAX_ENTRIES];
static uwlkv_key             used_entries;
static uwlkv_offset          next_block;

/**
 * @brief	Scans a memory block and indexes its content to uwlkv_entries. It uses linear search
 * 			and stops on a first free memory block. Block considered free if all of its bytes are
 * 			equal to UWLKV_ERASED_BYTE_VALUE.
 *
 * @returns	Number of keys loaded.
 */
uwlkv_key uwlkv_load_map(void)
{
    next_block = 0;
    used_entries = 0;

    uwlkv_offset offset;
    for (offset = 0; (offset + UWLKV_ENTRY_SIZE) <= nvram_interface.size; offset += UWLKV_ENTRY_SIZE)
    {
        uwlkv_key key;
        uwlkv_value value;
        uwlkv_error ret = uwlkv_read_entry(offset, &key, &value);

        if (UWLKV_E_NOT_EXIST == ret)
        {
            break;
        }

        if (UWLKV_E_SUCCESS == ret)
        {
            uwlkv_update_entry(key, offset);
        }
    }
    next_block = offset;

    return used_entries;
}

/**
 * @brief	Temporarly loads all values into memory, erases NVRAM and writing data back. The
 * 			purpose is to erase whole NVRAM and start from it's beginning. If this process
 * 			interrupted, data lost occurs. To prevent this, we can write a copy in a reserved
 * 			space in NVRAM.
 */
static void restart_map(void)
{
    uwlkv_value uwlkv_values[UWLKV_MAX_ENTRIES];

    for(uwlkv_key i = 0; i < used_entries; i++)
    {
        uwlkv_entry * entry = &uwlkv_entries[i];
        uwlkv_key key;
        uwlkv_value value;
        uwlkv_read_entry(entry->offset, &key, &value);
        uwlkv_values[i] = value;
    }

    nvram_interface.erase();
    next_block = 0;

    for(uwlkv_key i = 0; i < used_entries; i++)
    {
        uwlkv_entry * entry = &uwlkv_entries[i];
        uwlkv_value value   = uwlkv_values[i];
        entry->offset   = next_block;
        uwlkv_write_entry(next_block, entry->key, value);
        next_block += UWLKV_ENTRY_SIZE;
    }
}

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

/**
 * @brief	Reserves memory for one data block and returns an offset to its first byte. If all
 * 			NVRAM is used, it would be erased.
 *
 * @returns	Starting position of new block.
 */
uwlkv_offset uwlkv_get_next_block(void)
{
    if ((next_block + UWLKV_ENTRY_SIZE) > nvram_interface.size)
    {
        restart_map();
    }

    next_block += UWLKV_ENTRY_SIZE;

    return next_block - UWLKV_ENTRY_SIZE;
}
