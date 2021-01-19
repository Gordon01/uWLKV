[![Build Status](https://travis-ci.com/Gordon01/uWLKV.svg?branch=master)](https://travis-ci.com/Gordon01/uWLKV)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=Gordon01_uWLKV&metric=code_smells)](https://sonarcloud.io/dashboard?id=Gordon01_uWLKV)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=Gordon01_uWLKV&metric=coverage)](https://sonarcloud.io/dashboard?id=Gordon01_uWLKV)

# uWLKV
Micro wear-leveling key-value storage library for microcontrollers.

This library greatly simplifies the implementation of the wear-leveling technique. Most microcontrollers have a small amount of NVRAM (either Flash or EEPROM) presented as an array of bytes and the endurance of each byte is often limited by 100k writes, which is sometimes inconvenient of you need to save something frequently or you don't want to limit user's actions. However, if EEPROM usage is low, we can use its free space to level wear by not writing twice to the same position. 

The idea behind this library is to make all write into the next free NVRAM block. By doing this we wear out all available NVRAM bytes evenly. This is particularly efficient if only a few parameters are changed often because they will have the most available free space. If there is no free space to save another parameter, the whole NVRAM erased, defragmented and writing will continue from the beginning, after restoring currently saved parameters. This is particularly handy because some microcontrollers can only erase big pages in FLASH and we are not limited by this.

This comes with some overhead, of course, because we need to store a key alongside a value in NVRAM.

# How to use
You need to provide four functions to access your particular type of NVRAM and declare its size.

```
interface.read          = &flash_read;
interface.write         = &flash_write;
interface.erase_main    = &flash_erase_main;
interface.erase_reserve = &flash_erase_reserve;
interface.size          = 1024;
interface.reserved      = 256;

uwlkv_init(&interface);
```
Erase functions do not accept parameters and should erase corresponding NVRAM area:
* `erase_main` should erase memory from byte 0 to (`size` - `reserved` - 1)
* `erase_reserve` should erase memory from byte (`size` - `reserved`) to (`size` - 1)

Read and write functions have similar prototypes:
```CPP
int(* read)(uint8_t * data, uwlkv_offset start, uwlkv_offset size);
int(* write)(uint8_t * data, uwlkv_offset start, uwlkv_offset size);
```
Where:
* `data` - buffer pointer
* `start` - offset in bytes
* `size` - the amount of data to be manipulated, should be equal to the size of the buffer

Offset should start from 0.

Reserved area should have enough space just to fit `UWLKV_MAX_ENTRIES` plus metadata. It is only used during main area erase to have a backup in case of sudden reset. If you have Flash memory, which can only be erased by a whole page, you probably want to set reserved space to fit exactly one page.

If the supplied size looks wrong, `uwlkv_init()` will return 0 otherwise, it returns a maximum number of entries that can fit main area. You can calculate the approximate wear-leveling factor by dividing this value by `UWLKV_MAX_ENTRIES`.

After successeful initialization, use the following functions to manipulate your data:
```CPP
uwlkv_error uwlkv_get_value(uwlkv_key key, uwlkv_value * value);
uwlkv_error uwlkv_set_value(uwlkv_key key, uwlkv_value value);
```
Currently, the library supports only one type: `uwlkv_value` which is `int32_t` by default.

If your NVRAM type has a different value of erased byte (not 0xFF), please set it in `uwlkv.h` `UWLKV_ERASED_BYTE_VALUE`.

**You can only store a limited number of parameters, identified by unique keys, not by the size of your NVRAM**. By default, `UWLKV_MAX_ENTRIES` value is 20.

# Tuning
You can reduce RAM usage or overhead by adjusting values in `uwlkv.h`.

The library needs cache, which is currently a static array. If you do not need many unique keys, reduce `UWLKV_MAX_ENTRIES` value. 

To reduce storage overhead, you can make `uwlkv_key` or `uwlkv_value` type smaller, if you do not need to have keys larger than 255 or big values.

To reduce RAM usage by the cache, you may also make `uwlkv_offset` smaller. If you only have 64k or smaller, use `uint16_t`. You may also need to make struct `uwlkv_entry` packed.

# TODO
- [ ] Make an option to have cache size dynamic
- [ ] Support different types of values
