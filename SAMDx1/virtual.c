/*
 * sample minimal emulation of empty virtual floppy
 *
 * Copyright (c) 2018, Peter Lawrence
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "usb.h"

#define LSBFIRST_UINT16(x) (uint8_t)((x) >> 0), (uint8_t)((x) >> 8)

void usb_cbi_write_callback(uint32_t lba, const uint8_t *data)
{
  (void)lba;
  (void)data;

  /* throw away any data written */
}

static const uint8_t boot_sector[] =
{
  0xEB, 0x3C, 0x90, /* jump instruction */
  'M', 'S', 'D', 'O', 'S', '5', '.', '0', /* OEM */
  LSBFIRST_UINT16(USB_CBI_SECTOR_SIZE), /* bytes per sector */
  0x01, /* sectors per cluster */
  0x01, 0x00, /* reserved sectors */
  0x02, /* number of FATs */
  0xE0, 0x00, /* maximum number of root directory entries */
  LSBFIRST_UINT16(USB_CBI_TOTAL_BLOCKS), /* total sector count */
  USB_CBI_MEDIA_DESCRIPTOR_TYPE, /* media descriptor byte */
  0x09, 0x00, /* sectors per FAT */
  0x12, 0x00, /* sectors per track */
  0x02, 0x00, /* number of heads */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x29, /* boot signature */
  0xD4, 0xC3, 0xB2, 0xA1, /* Volume Serial Number (displayed right to left) */
  'N', 'O', ' ', 'N', 'A', 'M', 'E', ' ', ' ', ' ', ' ', /* Volume Name */
  'F', 'A', 'T', '1', '2',  ' ', ' ', ' ',
};

void usb_cbi_read_callback(uint32_t lba, uint8_t *data)
{
  (void)lba;
  (void)data;

  memset(data, 0x00, USB_CBI_SECTOR_SIZE);

  if (0 == lba)
  {
    /* first sector must look a very particular way */
    memcpy(data, boot_sector, sizeof(boot_sector));
    data[510] = 0x55;
    data[511] = 0xAA;
  }
  else if ((1 == lba) || (10 == lba))
  {
    /* the FAT must resemble something empty */
    data[0] = 0xF0;
    data[1] = 0xFF;
    data[2] = 0xFF;
  }
}
