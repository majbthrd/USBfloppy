/*
 * USB floppy (Mass Storage Subclass 04h CBI transport) for SAMD11/SAMD21
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

#ifndef _USB_CBI_H_
#define _USB_CBI_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "utils.h"
#include "usb_std.h"

/*- Definitions -------------------------------------------------------------*/

/* the CBI standard provides for either Control/Bulk/Interrupt or Control/Bulk transports */
#define CBI_NOT_CB

/* uncomment only one of these two to indicate single density or double density 3.5" floppy */
#define USB_CBI_MEDIUM_TYPE_CODE USB_FDU_1440KB
//#define USB_CBI_MEDIUM_TYPE_CODE USB_FDU_720KB

#define USB_CBI_TOTAL_BLOCKS          ( (USB_FDU_720KB == USB_CBI_MEDIUM_TYPE_CODE) ? 1440 : 2880 )
#define USB_CBI_MEDIA_DESCRIPTOR_TYPE ( (USB_FDU_720KB == USB_CBI_MEDIUM_TYPE_CODE) ? 0xF9 : 0xF0 )
#define USB_CBI_MEDIUM_TYPE           ( (USB_FDU_720KB == USB_CBI_MEDIUM_TYPE_CODE) ? 0x1E : 0x94 )
#define USB_CBI_SECTOR_SIZE           512

enum
{
  USB_UFI_FORMAT_UNIT                  = 0x04,
  USB_UFI_INQUIRY                      = 0x12,
  USB_UFI_MODE_SELECT                  = 0x55,
  USB_UFI_MODE_SENSE                   = 0x5A,
  USB_UFI_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E,
  USB_UFI_READ10                       = 0x28,
  USB_UFI_READ12                       = 0xA8,
  USB_UFI_READ_CAPACITY                = 0x25,
  USB_UFI_READ_FORMAT_CAPACITIES       = 0x23,
  USB_UFI_REQUEST_SENSE                = 0x03,
  USB_UFI_REZERO_UNIT                  = 0x01,
  USB_UFI_SEEK                         = 0x2B, /* not mandatory as READ, WRITE, FORMAT have an address */
  USB_UFI_SEND_DIAGNOSTIC              = 0x1D,
  USB_UFI_START_STOP_UNIT              = 0x1B,
  USB_UFI_TEST_UNIT_READY              = 0x00,
  USB_UFI_VERIFY                       = 0x2F,
  USB_UFI_WRITE10                      = 0x2A,
  USB_UFI_WRITE12                      = 0xAA,
  USB_UFI_WRITE_AND_VERIFY             = 0x2E,
};

enum
{
  USB_FDU_720KB    = 0x1E,
  USB_FDU_1440KB   = 0x94,
};

/*- Types -------------------------------------------------------------------*/

/*- Prototypes --------------------------------------------------------------*/
void usb_cbi_init(void);

void usb_cbi_write_callback(uint32_t lba, const uint8_t *data);
void usb_cbi_read_callback(uint32_t lba, uint8_t *data);

#endif // _USB_CBI_H_
