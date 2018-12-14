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

/*- Includes ----------------------------------------------------------------*/
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>
#include "utils.h"
#include "usb.h"
#include "usb_std.h"
#include "usb_cbi.h"
#include "usb_descriptors.h"

/*- Definitions -------------------------------------------------------------*/
#define STATUS_SENSE_KEY 0x00 /* NO SENSE (medium present and OK) */
#define STATUS_ASC       0x00
#define STATUS_ASCQ      0x00

#define MSBFIRST_UINT32(x)  (uint8_t)((x) >> 24), (uint8_t)((x) >> 16), (uint8_t)((x) >> 8),  (uint8_t)((x) >> 0)
#define MSBFIRST_UINT24(x)  (uint8_t)((x) >> 16), (uint8_t)((x) >> 8),  (uint8_t)((x) >> 0)

/*- Prototypes --------------------------------------------------------------*/
static void usb_cbi_ep_in_callback(int size);
static void usb_cbi_ep_out_callback(int size);

/*- Variables ---------------------------------------------------------------*/
static alignas(4) uint8_t sector_buffer[USB_CBI_SECTOR_SIZE];
static alignas(4) uint8_t response_buffer[64];
static alignas(4) uint8_t interrupt_message[2] = 
{
  STATUS_ASC, /* ASC */
  STATUS_ASCQ, /* ASCQ */
};

static uint32_t read_sectors_remaining, lba, write_sectors_remaining;

/* provide the same information in lots of slightly different ways... joy */

static const uint8_t inquiry[36] =
{
  0x00, /* direct-access device (floppy) */
  0x80, /* RMB = '1' (removable media) */
  0x00, /* "these fields shall be zero for the UFI device" */
  0x01, /* a value of 01h shall be used for the UFI device */
  0x1F, /* Additional Length (31) */
  0x00, 0x00, 0x00, /* reserved */
  'A',  'c',  'm',  'e',  ' ',  ' ',  ' ',  ' ',  /* Vendor Information (8 bytes) */
  'U',  'S',  'B',  '-',  'F',  'D',  'D',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  /* Product Identification (16 bytes) */
  '1',  '.',  '0',  '0',  /* Product Revision Level (n.nn) */
};

static const uint8_t request_sense[18] =
{
  0x70, /* "Error Code" (to indicate that this message's type) */
  0x00, /* Reserved */
  STATUS_SENSE_KEY, /* Sense Key */
  0x00, 0x00, 0x00, 0x00, /* Information */
  0x0A, /* Additional Sense Length (10) */
  0x00, 0x00, 0x00, 0x00, /* Reserved */
  STATUS_ASC, /* ASC */
  STATUS_ASCQ, /* ASCQ */
  0x00, 0x00, 0x00, 0x00, /* Reserved */
};

static const uint8_t capacity[8] =
{
  MSBFIRST_UINT32(USB_CBI_TOTAL_BLOCKS - 1),
  0x00, 
  MSBFIRST_UINT24(USB_CBI_SECTOR_SIZE),
};

static const uint8_t format_capacities[12] =
{
  0x00, 0x00, 0x00, /* Reserved */
  0x08, /* Capacity List Length */
  MSBFIRST_UINT32(USB_CBI_TOTAL_BLOCKS),
  0x02, /* formatted media - current media capacity */
  MSBFIRST_UINT24(USB_CBI_SECTOR_SIZE),
};

static const uint8_t mode_sense[8] =
{
  0x00, 0x06, 
  USB_CBI_MEDIUM_TYPE,  /* Medium Type Code */
  0x00, /* 00h: write-allowed, 80h: write-protected */
  0x00, 0x00, 0x00, 0x00, /* Reserved */
};

/*- Implementations ---------------------------------------------------------*/

void usb_cbi_init(void)
{
  usb_set_callback(USB_FLOPPY_EP_IN, usb_cbi_ep_in_callback);
  usb_set_callback(USB_FLOPPY_EP_OUT, usb_cbi_ep_out_callback);
}

static void generate_interrupt(void)
{
#ifdef CBI_NOT_CB
  usb_send(USB_FLOPPY_EP_NOTIFY, interrupt_message, sizeof(interrupt_message));
#endif
}

static void usb_cbi_ep_in_callback(int size)
{
  (void)size;

  if (read_sectors_remaining)
  {
    usb_cbi_read_callback(lba++, sector_buffer);
    usb_send(USB_FLOPPY_EP_IN, sector_buffer, USB_CBI_SECTOR_SIZE);
    read_sectors_remaining--;
    if (!read_sectors_remaining)
      generate_interrupt();
  }
}

static void usb_cbi_ep_out_callback(int size)
{
  (void)size;

  if (write_sectors_remaining)
  {
    usb_cbi_write_callback(lba, sector_buffer);
    write_sectors_remaining--;
    if (!write_sectors_remaining)
      generate_interrupt();
  }
  usb_recv(USB_FLOPPY_EP_OUT, sector_buffer, USB_CBI_SECTOR_SIZE);
}

static void send_response(const uint8_t *data, int size)
{
  if (size)
    memcpy(response_buffer, data, size);
  usb_send(USB_FLOPPY_EP_IN, response_buffer, size);
  generate_interrupt();
}

static void usb_floppy_set(uint8_t *data, int size)
{
  (void)size;

  uint32_t speculative_lba, speculative_sectors;

  speculative_lba  = (uint32_t)data[2] << 24;
  speculative_lba |= (uint32_t)data[3] << 16;
  speculative_lba |= (uint32_t)data[4] << 8;
  speculative_lba |= (uint32_t)data[5] << 0;

  speculative_sectors = (uint32_t)data[7] << 8;
  speculative_sectors |= (uint32_t)data[8] << 0;

  switch (data[0])
  {
  case USB_UFI_INQUIRY:
    send_response(inquiry, sizeof(inquiry));
    break;
  case USB_UFI_REQUEST_SENSE:
    send_response(request_sense, sizeof(request_sense));
    break;
  case USB_UFI_READ_CAPACITY:
    send_response(capacity, sizeof(capacity));
    break;
  case USB_UFI_READ_FORMAT_CAPACITIES:
    send_response(format_capacities, sizeof(format_capacities));
    break;
  case USB_UFI_MODE_SENSE:
    send_response(mode_sense, sizeof(mode_sense));
    break;
  case USB_UFI_READ10:
    lba = speculative_lba;
    read_sectors_remaining = speculative_sectors;
    usb_cbi_ep_in_callback(0);
    break;
  case USB_UFI_WRITE10:
    lba = speculative_lba;
    usb_cbi_ep_out_callback(0);
    write_sectors_remaining = speculative_sectors;
    /* deliberate fall through */
  default:
    send_response(NULL, 0);
  }
}

bool usb_class_handle_request(usb_request_t *request)
{
  if (USB_OUT_ENDPOINT == (request->bmRequestType & USB_DIRECTION_MASK))
  {
    usb_control_recv(usb_floppy_set);
    return true;
  }

  return false;
}

void usb_configuration_callback(int config)
{
  (void)config;
  read_sectors_remaining = 0;
  write_sectors_remaining = 0;
}
