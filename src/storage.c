#include "uwlkv.h"
#include "entry.h"
#include "map.h"
#include "storage.h"

extern uwlkv_nvram_interface nvram_interface;
static uwlkv_offset          next_block;

static uwlkv_nvram_state get_nvram_state(void);
static void load_map(void);
static void prepare_for_first_use(void);
static void recover_after_iterrupted_main_erase(void);
static void recover_after_interrupted_reserve_erase(void);
static void prepare_area(uwlkv_area area);
static void transfer_main_to_reserve(void);
static void transfer_reserve_to_main(void);

void uwlkv_cold_boot(void)
{
    uwlkv_reset_map();

    const uwlkv_nvram_state nvram_state = get_nvram_state();
    switch (nvram_state)
    {
    case UWLKV_S_CLEAN:
        load_map();
        break;

    case UWLKV_S_BLANK:
        prepare_for_first_use();
        break;
    
    case UWLKV_S_MAIN_ERASE_INTERRUPTED:
        recover_after_iterrupted_main_erase();
        break;

    case UWLKV_S_RESERVE_ERASE_INTERRUPTED:
        recover_after_interrupted_reserve_erase();
        break;
    }
}

/**
 * @brief	Scans a memory block and indexes its content to uwlkv_entries. It uses linear search
 * 			and stops on a first free memory block. Block considered free if all of its bytes are
 * 			equal to UWLKV_ERASED_BYTE_VALUE.
 *
 * @returns	Number of keys loaded.
 */
static void load_map(void)
{
    uwlkv_reset_map();

    uwlkv_offset offset;
    for (offset =  UWLKV_METADATA_SIZE; 
        (offset +  UWLKV_ENTRY_SIZE) <= (nvram_interface.size - nvram_interface.reserved); 
         offset += UWLKV_ENTRY_SIZE)
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
}

static void prepare_for_first_use(void)
{
    nvram_interface.erase_main();
    nvram_interface.erase_reserve();

    uint8_t main_metadata[UWLKV_METADATA_SIZE] = { UWLKV_NVRAM_ERASE_STARTED, UWLKV_NVRAM_ERASE_FINISHED };
    nvram_interface.write(main_metadata, 0, UWLKV_METADATA_SIZE);

    next_block = UWLKV_METADATA_SIZE;
}

static void recover_after_iterrupted_main_erase(void)
{
    nvram_interface.erase_main();
    transfer_reserve_to_main();
    prepare_area(UWLKV_RESERVED);
}

static void recover_after_interrupted_reserve_erase(void)
{
    nvram_interface.erase_reserve();
    transfer_main_to_reserve();
    prepare_area(UWLKV_MAIN);
}

static inline uwlkv_offset get_reserve_offset(uwlkv_offset offset)
{
    return nvram_interface.size - nvram_interface.reserved + offset;
}

static void transfer_reserve_to_main(void)
{
    const uwlkv_offset reserve_offset = get_reserve_offset(0);

    uwlkv_offset offset;
    for (offset =  UWLKV_METADATA_SIZE; 
        (offset +  UWLKV_ENTRY_SIZE) <= (nvram_interface.reserved); 
         offset += UWLKV_ENTRY_SIZE)
    {
        uwlkv_key key;
        uwlkv_value value;
        uwlkv_error ret = uwlkv_read_entry(reserve_offset + offset, &key, &value);

        if (UWLKV_E_NOT_EXIST == ret)
        {
            break;
        }

        if (UWLKV_E_SUCCESS == ret)
        {
            uwlkv_write_entry(offset, key, value);
            uwlkv_update_entry(key, offset);
        }
    }
    
    next_block = offset;
}

static void transfer_main_to_reserve(void)
{
    uwlkv_offset reserve_offset = get_reserve_offset(UWLKV_METADATA_SIZE);
    for(uwlkv_key i = 0; i < uwlkv_get_used_entries(); i++)
    {
        uwlkv_entry * entry = uwlkv_get_entry_by_id(i);
        uwlkv_key key;
        uwlkv_value value;
        uwlkv_read_entry(entry->offset, &key, &value);
        uwlkv_write_entry(reserve_offset, key, value);
        reserve_offset += UWLKV_ENTRY_SIZE;
    }
}

static uwlkv_nvram_state get_nvram_state(void)
{
    uint8_t main_metadata[UWLKV_METADATA_SIZE], reserve_metadata[UWLKV_METADATA_SIZE];
    nvram_interface.read(main_metadata,    0,                     UWLKV_METADATA_SIZE);
    nvram_interface.read(reserve_metadata, get_reserve_offset(0), UWLKV_METADATA_SIZE);

    const uint8_t main_started     = UWLKV_NVRAM_ERASE_STARTED  == reserve_metadata[UWLKV_O_ERASE_STARTED];
    const uint8_t reserve_started  = UWLKV_NVRAM_ERASE_STARTED  == main_metadata[UWLKV_O_ERASE_STARTED];
    const uint8_t main_finished    = UWLKV_NVRAM_ERASE_FINISHED == reserve_metadata[UWLKV_O_ERASE_FINISHED];
    const uint8_t reserve_finished = UWLKV_NVRAM_ERASE_FINISHED == main_metadata[UWLKV_O_ERASE_FINISHED];
    const uint8_t main_clean       = uwlkv_is_block_erased(main_metadata, UWLKV_METADATA_SIZE);
    const uint8_t reserve_clean    = uwlkv_is_block_erased(reserve_metadata, UWLKV_METADATA_SIZE);

    if (reserve_finished && reserve_clean)
    {
        return UWLKV_S_CLEAN;
    }

    if (    (main_clean   && main_started)
        ||  (main_started && !main_finished) )
    {
        return UWLKV_S_MAIN_ERASE_INTERRUPTED;
    }

    if (    (reserve_finished && !reserve_clean)
        ||  (reserve_started  && !reserve_finished) )
    {
        return UWLKV_S_RESERVE_ERASE_INTERRUPTED;
    }

    return UWLKV_S_BLANK;
}

/**
 * @brief	Erases specified area with progress indication to others area metadata.
 */
static void prepare_area(uwlkv_area area)
{
    uint8_t operation_flag = UWLKV_NVRAM_ERASE_STARTED;
    uwlkv_offset base_address = get_reserve_offset(0);
    uwlkv_erase  erase_function = nvram_interface.erase_main;

    if (UWLKV_RESERVED == area)
    {
        base_address   = 0;
        erase_function = nvram_interface.erase_reserve;
    }
    
    nvram_interface.write(&operation_flag, base_address, 1);
    erase_function();
    operation_flag = UWLKV_NVRAM_ERASE_FINISHED;
    nvram_interface.write(&operation_flag, base_address + 1, 1);
}

/**
 * @brief	Temporarly loads all values into memory, erases NVRAM and writing data back. The
 * 			purpose is to erase whole NVRAM and start from it's beginning. If this process
 * 			interrupted, data lost occurs. To prevent this, we can write a copy in a reserved
 * 			space in NVRAM.
 */
static void restart_map(void)
{
    transfer_main_to_reserve();
    prepare_area(UWLKV_MAIN);
    transfer_reserve_to_main();
    prepare_area(UWLKV_RESERVED);
}

/**
 * @brief	Reserves memory for one data block and returns an offset to its first byte. If all
 * 			NVRAM is used, it would be erased.
 *
 * @returns	Starting position of new block.
 */
uwlkv_offset uwlkv_get_next_block(void)
{
    if ((next_block + UWLKV_ENTRY_SIZE) > (nvram_interface.size - nvram_interface.reserved))
    {
        restart_map();
    }

    next_block += UWLKV_ENTRY_SIZE;

    return next_block - UWLKV_ENTRY_SIZE;
}