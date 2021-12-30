/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _FLASH_H_
#define _FLASH_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "pico/stdlib.h"

#define SECTOR_SIZE 4096 
#define FATFS_OFFSET (1 * 1024 * 1024)
#define FATFS_SIZE (PICO_FLASH_SIZE_BYTES - FATFS_OFFSET)

void flash_erase(uint32_t add, uint32_t len);
void flash_read (uint32_t addr, void* buffer, uint32_t len);
void flash_write(uint32_t addr, void const *data, uint32_t len);
void flash_flush(void);

#ifdef __cplusplus
 }
#endif

#endif /* _FLASH_H_ */
