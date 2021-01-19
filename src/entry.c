/* This module accesess NVRAM and serializes/deserializes data.
 */

#include "uwlkv.h"
#include "entry.h"

extern uwlkv_nvram_interface uwlkv_nvram;
extern uwlkv_cache_interface uwlkv_cache;

/**
 * @brief	Read entry from NVRAM by offset.
 *
 * @param 	   	offset	Offset in bytes.
 * @param [out]	key   	Entry key.
 * @param [out]	value 	Entry value.
 *
 * @returns	UWLKV_E_SUCCESS on successeful read.
 */
uwlkv_error uwlkv_read_entry(const uwlkv_offset offset, uwlkv_key * key, uwlkv_value * value)
{
    if ((offset + UWLKV_ENTRY_SIZE) > uwlkv_nvram.size)
    {
        return UWLKV_E_WRONG_OFFSET;
    }

    uint8_t block[UWLKV_ENTRY_SIZE];
    if (uwlkv_nvram.read((uint8_t *)&block, offset, UWLKV_ENTRY_SIZE))
    {
        return UWLKV_E_NVRAM_ERROR;
    }

    if (uwlkv_is_block_erased(block, UWLKV_ENTRY_SIZE))
    {
        return UWLKV_E_NOT_EXIST;
    }

    *key      = *(uwlkv_key*)&block[0];
    *value    = *(uwlkv_value*)&block[sizeof(uwlkv_key)];

    return UWLKV_E_SUCCESS;
}

/**
 * @brief	Write entry to NVRAM by offset.
 *
 * @param 	offset	Offset in bytes.
 * @param 	key   	Entry key.
 * @param 	value 	Entry value.
 *
 * @returns	UWLKV_E_SUCCESS on successeful write.
 */
uwlkv_error uwlkv_write_entry(uwlkv_offset offset, uwlkv_key key, uwlkv_value value)
{
    if ((offset + UWLKV_ENTRY_SIZE) > uwlkv_nvram.size)
    {
        return UWLKV_E_WRONG_OFFSET;
    }

    uint8_t block[UWLKV_ENTRY_SIZE];
    uwlkv_key * key_in_block       = (uwlkv_key*)&block[0];
    uwlkv_value * value_in_block   = (uwlkv_value*)&block[sizeof(uwlkv_key)];

    *key_in_block   = key;
    *value_in_block = value;

    if (uwlkv_nvram.write((uint8_t *)&block, offset, UWLKV_ENTRY_SIZE))
    {
        return UWLKV_E_NVRAM_ERROR;
    }

    return UWLKV_E_SUCCESS;
}

/**
 * @brief	Checks that given block is fully erased (filled with UWLKV_ERASED_BYTE_VALUE)
 *
 * @param [in]	data	Data to be tested.
 * @param 	  	size	Size of data (in bytes).
 *
 * @returns	- 0 block is not erased
 * 			- 1 block is erased.
 */
uint8_t uwlkv_is_block_erased(const uint8_t * data, const uwlkv_offset size)
{
    uint8_t erased_bytes = 0;
    for (uwlkv_offset i = 0; i < size; i++)
    {
        if (data[i] == UWLKV_ERASED_BYTE_VALUE)
        {
            erased_bytes += 1;
        }
    }

    return erased_bytes == size;
}
