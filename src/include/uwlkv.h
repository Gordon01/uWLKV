#ifndef UWLKV_NVRAM_LIB
#define UWLKV_NVRAM_LIB

#include <stdint.h>

/* You can change a storage type if you don't need big values for key or value to save some space */
typedef uint16_t uwlkv_key;                        /* Record key */
typedef int32_t  uwlkv_value;                      /* Record value */
typedef uint32_t uwlkv_offset;                     /* NVRAM address. Can be reduced to match memory size and save some RAM */
typedef int(* uwlkv_erase)(void);                  /* NVRAM erase function prototype */
typedef uint8_t * (* uwlkv_double_capacity)(void); /* Cache double function prototype */


#define UWLKV_O_ERASE_STARTED       (0)            /* Offset of ERASE_STARTED flag */
#define UWLKV_O_ERASE_FINISHED      (1)            /* Offset of ERASE_FINISHED flag */
#define UWLKV_METADATA_SIZE         (2)            /* Number of bytes, that library use in the beginning of each area */
#define UWLKV_NVRAM_ERASE_STARTED   (0xE2)         /* Magic for ERASE_STARTED flag */
#define UWLKV_NVRAM_ERASE_FINISHED  (0x3E)         /* Magic for ERASE_FINISHED flag */

#define UWLKV_ENTRY_SIZE            (sizeof(uwlkv_key) + sizeof(uwlkv_value))
#define UWLKV_MINIMAL_SIZE          (UWLKV_ENTRY_SIZE + UWLKV_METADATA_SIZE)
#define UWLKV_MAX_ENTRIES           (20)           /* Maximum amount of unique keys. Increases RAM consumption */
#define UWLKV_ERASED_BYTE_VALUE     (0xFF)         /* Value of erased byte of NVRAM */

typedef struct
{
    uwlkv_key      key;
    uwlkv_offset   offset;
} uwlkv_entry;

/* You must provide an interface to access storage device. 
 * Read/write functions should use logical address (starting from 0).
 * erase_main() should erase a main (large) area, without touching reserved area.
 * erase_reserve() should erase only a reserved area.
 */
typedef struct
{
	int(* read)(uint8_t * data, uwlkv_offset start, uwlkv_offset size);
	int(* write)(uint8_t * data, uwlkv_offset start, uwlkv_offset size);
    uwlkv_erase  erase_main;
    uwlkv_erase  erase_reserve;
    uwlkv_offset size;                  /* Total size of provided memory */
    uwlkv_offset reserved;              /* Reserved area size in that memory */
} uwlkv_nvram_interface;

/* You also need to choose the way to cache entry position. You can either:
 * - disable cache entirely, by passing NULL as .cache value,
 * - have static cache, by passing NULL as .double_capacity value,
 * - have dynamic cache, by providing cache doubling function double_capacity().
 */
typedef struct
{
    uint8_t * cache;                    /* Pointer to entry cache */
    uwlkv_key capacity;                 /* Capacity of the cache in uwlkv_entry number */
    uwlkv_double_capacity double_capacity;
} uwlkv_cache_interface;

typedef enum
{
    UWLKV_E_SUCCESS,        
    UWLKV_E_NOT_EXIST,                  /* Requested entry does not exist on NVRAM */
    UWLKV_E_NVRAM_ERROR,                /* NVRAM interface signalled an error during the operation */
    UWLKV_E_NOT_STARTED,                /* UWLKV haven't been initialized */
    UWLKV_E_NO_SPACE,                   /* No free space in map for new entry */
    UWLKV_E_WRONG_OFFSET,               /* Provided offset is out of NVRAM bounds */
} uwlkv_error;

typedef enum
{
    UWLKV_MAIN,
    UWLKV_RESERVED
} uwlkv_area;

typedef enum
{
    UWLKV_S_BLANK,                      /* NVRAM is fully erased (new) */
    UWLKV_S_CLEAN,                      /* Last shutdown was clean */
    UWLKV_S_MAIN_ERASE_INTERRUPTED,     /* Main area erase was interrupted */
    UWLKV_S_RESERVE_ERASE_INTERRUPTED   /* Reserved area erase was interrupted */
} uwlkv_nvram_state;

#ifdef __cplusplus
extern "C" {
#endif
    
    uwlkv_offset uwlkv_init(const uwlkv_nvram_interface * nvram_interface, 
                            const uwlkv_cache_interface * cache_interface);
    uwlkv_key uwlkv_get_entries_number(void);
    uwlkv_key uwlkv_get_free_entries(void);
    uwlkv_error uwlkv_get_value(uwlkv_key key, uwlkv_value * value);
    uwlkv_error uwlkv_set_value(uwlkv_key key, uwlkv_value value);

#ifdef __cplusplus
}
#endif

#endif
