[![Build Status](https://travis-ci.com/Gordon01/uWLKV.svg?branch=master)](https://travis-ci.com/Gordon01/uWLKV)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=Gordon01_uWLKV&metric=code_smells)](https://sonarcloud.io/dashboard?id=Gordon01_uWLKV)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=Gordon01_uWLKV&metric=coverage)](https://sonarcloud.io/dashboard?id=Gordon01_uWLKV)

# uWLKV
Micro write-leveling key-value storage library for microcontrollers.

This library greatly simplifies implementation of write-leveling technique. Most microcontrollers have small amount of EEPROM presented as array of bytes and endurance of each byte is often limited by 100k writes, which is sometimes inconvenient of you need to save something frequently or you don't want to limit user's actions. However, if EEPROM usage is low, we can use its free space to level a wear by not writing twice to the same position. 

Idea behind this library is to make all writes into a next free EEPROM block. By doing this we wear out all available EEPROM bytes evenly. This is particularly efficient if only a few parameters changed often, because they will have most available free space. If there is no free space to save another parameter, whole EEPROM erased, and writing will continue from the beginning, after restoring currently saved parameters. This is particularly handy because some microcontrollers can only erase big pages in FLASH and we are not limited by this.

This comes with some overhead, of course, because we need to store a key in EEPROM.

# How to use
You need to provide three functions to access you particular type of NVRAM and declare its size.

```
uwlkv_nvram_interface interface;
interface.read  = &flash_read;
interface.write = &flash_write;
interface.erase = &flash_erase;
interface.size  = 1024;

uwlkv_init(&interface);
```
Read and write functions has similar prototypes:
```CPP
	int(* read)(uint8_t * data, uwlkv_offset start, uwlkv_offset size);
	int(* write)(uint8_t * data, uwlkv_offset start, uwlkv_offset size);
```
Where:
* data - buffer pointer
* start - offset in bytes
* size - amount of data to be readen or writen, should be equal to the size of buffer

Offset should start from 0.
Erase function do not accept parameters and should erase all memory available to library.

After successeful initialization, use the following functions to manipulate your data:
```CPP
uwlkv_error uwlkv_get_value(uwlkv_key key, uwlkv_value * value);
uwlkv_error uwlkv_set_value(uwlkv_key key, uwlkv_value value);
```
Currently, library supports only one type: `uwlkv_value` which is `int32_t` by default.

If your NVRAM type has different value of erased byte (not 0xFF), please set it in `uwlkv.h` `UWLKV_ERASED_BYTE_VALUE`.

**You can only store a limited number of parameters, identified by unique keys, not by size of your NVRAM**. By default, `UWLKV_MAX_ENTRIES` value is 20.

# Tuning
You can reduce RAM usage or overhead by adjusting values in `uwlkv.h`.

Library needs cache, which is currently a static array. If you do not need many unique keys, reduce `UWLKV_MAX_ENTRIES` value. 

To reduce storage overhead, you can make `uwlkv_key` or `uwlkv_value` type smaller, if you do not need to have keys larger than 255 or big values.

To reduce RAM usage by cache, you may also make `uwlkv_offset` smaller. If you only have 64k or smaller, use `uint16_t`. You may also need to make struct `uwlkv_entry` packed.

# TODO
- [ ] Make option to have cache size dynamic
- [ ] Support different types of values
