[![Build Status](https://travis-ci.com/Gordon01/uWLKV.svg?branch=master)](https://travis-ci.com/Gordon01/uWLKV)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=Gordon01_uWLKV&metric=code_smells)](https://sonarcloud.io/dashboard?id=Gordon01_uWLKV)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=Gordon01_uWLKV&metric=coverage)](https://sonarcloud.io/dashboard?id=Gordon01_uWLKV)

# uWLKV

Microcontroller Key–Value Storage with Wear-Leveling

This library makes implementing wear-leveling on microcontrollers effortless. Many MCUs provide only a small NVRAM area (Flash or EEPROM) organized as a byte array, and each byte typically endures ≈100 000 writes. If your application writes data frequently, or you don’t want to limit user actions, this write limit can be problematic. By spreading writes across unused space, wear-leveling extends the usable life of the memory without restricting functionality.

[Full project documentation on DeepWiki](https://deepwiki.com/Gordon01/uWLKV).

## How it works

* __Sequential writes__: Each new key–value record is appended to the next free NVRAM block rather than overwriting old data. This evens out wear across all bytes.
* __Dynamic space allocation__: Frequently changing parameters naturally occupy more free blocks, maximizing their available write-cycles without manual tuning.
* __Page-wise erase and defragmentation__: When no free blocks remain, the library performs a single erase of the entire NVRAM region (or Flash page), compacts the latest key–value pairs back to the start, and then resumes writing. This suits microcontrollers that can only erase large Flash pages and avoids per-parameter erases.

## Trade-offs

* __Space overhead__: Every record stores its key alongside its value.
* __Occasional latency__: Erase-and-compact cycles add a brief pause, but only when the main region is full.

# How to use

## Define your NVRAM interface

```cpp
uwlkv_nvram_interface interface;

interface.read          = &flash_read;
interface.write         = &flash_write;
interface.erase_main    = &flash_erase_main;
interface.erase_reserve = &flash_erase_reserve;
interface.size          = 1024;  // Total NVRAM size, in bytes
interface.reserved      = 256;   // Size of the reserved area within NVRAM, in bytes
```

### Read and write

```cpp
int(* read)(uint8_t * data, uwlkv_offset start, uwlkv_offset size);
int(* write)(uint8_t * data, uwlkv_offset start, uwlkv_offset size);
```

Where:
* `data` - buffer pointer
* `start` - byte-offset (0-based) into the NVRAM region
* `size` - number of bytes to read or write (must match buffer size)

### Erase

```cpp
int (*erase_main)(void); // Must erase bytes [ 0 .. size − reserved − 1 ]
int (*erase_reserve)(void); // Must erase bytes [ size − reserved .. size − 1 ]
```

### `reserved`

A backup area (in bytes) that holds up to `UWLKV_MAX_ENTRIES` plus metadata. During a full-region erase, the library uses this space to temporarily store valid entries in case of an unexpected reset. If your Flash can only erase in page-sized chunks, set reserved to exactly one page.

## Initialize

```cpp
int entries = uwlkv_init(&interface);
```

* Returns 0 if `size` or `reserved` values are invalid.
* Otherwise returns the maximum number of entries that can fit in the main area.

You can estimate the wear-leveling factor as

```
wear_factor ≃ (entries) / UWLKV_MAX_ENTRIES
```

## Store and retrieve values

After successful initialization, use:

```cpp
uwlkv_error uwlkv_get_value(uwlkv_key key, uwlkv_value *value_out);
uwlkv_error uwlkv_set_value(uwlkv_key key, uwlkv_value value_in);
```

* Keys are unique identifiers (e.g. integers or enums).
* Values default to `int32_t`.
* To change the erase-state byte from default `0xFF`, redefine `UWLKV_ERASED_BYTE_VALUE` in `uwlkv.h`.*

## Limits

The number of stored parameters is capped by `UWLKV_MAX_ENTRIES` (default 20), not by the raw NVRAM size.

# Tuning for Lower RAM and Storage Overhead

Adjust these in uwlkv.h to shrink RAM or NVRAM overhead:

* `UWLKV_MAX_ENTRIES`: Reduce if you need fewer unique keys to shrink the static cache.
* __Shrink key or value types__. By default, `uwlkv_key` is `uint16_t` and `uwlkv_value` is `int32_t`. If your keys never exceed 0–255, you can redefine `uwlkv_key` as `uint8_t`. Likewise, if stored values fit in 16 bits, redefine `uwlkv_value` as `int16_t` (or smaller).
* __Reduce offset width__. The type uwlkv_offset determines how you address bytes in NVRAM. If your total NVRAM size is ≤ 65 535 bytes, change `uwlkv_offset` to `uint16_t` instead of `uint32_t` to cut RAM used by index calculations.
