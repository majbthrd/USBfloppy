USBfloppy
=========

## Introduction

I couldn't find a USB floppy device implementation, so I wrote one.

This was written for the Atmel/Microchip SAMD11 and SAMD21 USB microcontrollers.

The implementation follows the [USB Mass Storage Class CBI Transport Specification](https://www.usb.org/sites/default/files/usb_msc_cbi_1.1.pdf) and [USB Mass Storage Class UFI Command Specification](https://usb.org/sites/default/files/usbmass-ufi10.pdf).

The source code provides just enough emulation of the file system to fool the PC host, but you could add your own non-volatile storage implementation by replacing virtual.c  (You'll also have to make your own 'chu-chunk, chu-chunk' sounds as you copy files if you want to hear that too.)

The USB stack source code owes its origins to [vcp](https://github.com/ataradov/vcp).

## Build Requirements

One approach is to use [Rowley Crossworks for ARM](http://www.rowley.co.uk/arm/) to compile this code.  It is not free software, but has been my favorite go-to ARM development tool for a decade and counting.

*OR*

Use the Makefile in the make subdirectory.  With this approach, the code can be built using only open-source software.  In Ubuntu-derived distributions, this is likely achieved with as little as:

```
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

## Testing

Testing was done with Linux, Windows 7, and Windows 10.

All versions of Windows require a .inf file for VID:PID values other the ones natively supported by the OS.  "Wait", I hear you say, "don't you know Windows support all Mass Storage devices regardless of VID:PID?"  The wisdom of the Internet is wrong.  Redmond may have given up trying to monetize MSC Subclass 06h, but their money-making machine apparatus is alive and kicking.
