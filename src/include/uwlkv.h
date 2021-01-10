#ifndef UWLKV_NVRAM_LIB
#define UWLKV_NVRAM_LIB

#include <stdint.h>

/* You can change a storage type if you don't need big values for key or value to save some space */
typedef uint16_t uwlkv_key;         /* Record key */
typedef int32_t  uwlkv_value;       /* Record value */
typedef uint32_t uwlkv_offset;      /* NVRAM address. Can be reduced to match memory size and save some RAM */

#define UWLKV_ENTRY_SIZE            (sizeof(uwlkv_key) + sizeof(uwlkv_value))
#define UWLKV_MAX_ENTRIES           (20)                    /* Maximum amount of unique keys. Increases RAM consumption */
#define UWLKV_ERASED_BYTE_VALUE     (0xFF)                  /* Value of erased byte of NVRAM */

typedef struct
{
    uwlkv_key      key;
    uwlkv_offset   offset;
} uwlkv_entry;

/* You must provide an interface to access storage device. 
 * Read/write functions should use logical address (starting from 0).
 * Erase should erase an entire memory region dedicated for a module.
 */
typedef struct
{
	int(* read)(uint8_t * data, uwlkv_offset start, uwlkv_offset size);
	int(* write)(uint8_t * data, uwlkv_offset start, uwlkv_offset size);
    int(* erase)(void);
    uwlkv_offset size;
} uwlkv_nvram_interface;

typedef enum
{
    UWLKV_E_SUCCESS,        
    UWLKV_E_NOT_EXIST,      /* Requested entry does not exist on NVRAM */
    UWLKV_E_NVRAM_ERROR,    /* NVRAM interface signalled an error during the operation */
    UWLKV_E_NOT_STARTED,    /* UWLKV haven't been initialized */
    UWLKV_E_NO_SPACE,       /* No free space in map for new entry */
    UWLKV_E_WRONG_OFFSET,   /* Provided offset is out of NVRAM bounds */
} uwlkv_error;

#ifdef __cplusplus
extern "C" {
#endif
    
    uwlkv_offset uwlkv_init(uwlkv_nvram_interface * nvram_interface);
    uwlkv_key uwlkv_get_entries_number(void);
    uwlkv_key uwlkv_get_free_entries(void);
    uwlkv_error uwlkv_get_value(uwlkv_key key, uwlkv_value * value);
    uwlkv_error uwlkv_set_value(uwlkv_key key, uwlkv_value value);

#ifdef __cplusplus
}
#endif

#endif
