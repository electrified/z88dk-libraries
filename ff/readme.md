## Introduction to the Z80 FatFS functions

FatFs is a generic FAT/exFAT filesystem module for small embedded systems. The FatFs module is written in compliance with ANSI C (C89) and completely separated from the disk I/O layer. Therefore it is independent of the platform. It can be incorporated into small microcontrollers with limited resource, such as 8051, PIC, AVR, ARM, Z80, RX and etc.

#### Features
<ul>
 <li>DOS/Windows compatible FAT/exFAT filesystem.</li>
 <li>Platform independent. Easy to port.</li>
 <li>Very small footprint for program code and work area.</li>
 <li>Various configuration options to support for:
  <ul>
   <li>Multiple volumes (physical drives and partitions).</li>
   <li>Multiple ANSI/OEM code pages including DBCS.</li>
   <li>Long file name in ANSI/OEM or Unicode.</li>
   <li>exFAT filesystem.</li>
   <li>RTOS envilonment.</li>
   <li>Fixed or variable sector size.</li>
   <li>Read-only, optional API, I/O buffer and etc...</li>
  </ul>
 </li>
</ul>

## Preparation

First configure the library to suit your requirements by adjusting the `source/ffconf.h` file to provide the functions you need.

The library can be compiled using the following command lines in Linux, with the `+target` modified to be relevant to your machine.

`zcc +target -lm -x -SO3 --opt-code-size -clib=sdcc_ix --max-allocs-per-node200000 @ff.lst -o ff`

`zcc +target -lm -x -SO3 --opt-code-size -clib=sdcc_iy --max-allocs-per-node200000 @ff.lst -o ff`

The resulting `ff.lib` file should be moved to `~/target/lib/newlib/sdcc_ix` or `~/target/lib/newlib/sdcc_iy` respectively.

The `z88dk-lib` function is used to install for the desired target. e.g.

```bash
cd ..
z88dk-lib +yaz180 ff
```

Some further examples of `z88dk-lib` usage.
list help
```bash
z88dk-lib
```
list 3rd party libraries already installed for the zx target
```bash
z88dk-lib +zx
```
remove the `libname1` `libname2` ... libraries from the zx target, -f for no nagging about deleting files.
```bash
z88dk-lib +zx -r -f libname1 libname2 ...
```

## Usage

Once installed, the FatFs library can be linked against on the compile line by adding `-llib/target/ff` and the include file can be found with `#include <lib/target/ff.h>`.

A simple usage example, for the `+yaz180` target.

```c
/*----------------------------------------------------------------------*/
/* Foolproof FatFs sample project for Z80              (C)ChaN, 2014    */
/*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arch/yaz180.h>
#include <arch/yaz180/system_time.h>

#include <lib/yaz180/time.h>   /* Declaration of system time */

#include "ffconf.h"
#include <lib/yaz180/ff.h>     /* Declarations of FatFs API */

// zcc +yaz180 -subtype=basic_dcio -v --list -m -SO3 --opt-code-size -clib=sdcc_iy -llib/yaz180/time -llib/yaz180/ff --max-allocs-per-node200000 ffmain.c -o ffmain -create-app

// doke &h2704, &2900 (Look for __Start symbol in ffmain.map)

static FATFS FatFs;		/* FatFs work area needed for each volume */
static FIL Fil;			/* File object needed for each open file */

int main (void)
{
	UINT bw;

    system_tick_init((void *)0x2044);
    set_zone((int32_t)10 * ONE_HOUR);   /* Australian Eastern Standard Time */
    set_system_time(1505695200 - UNIX_OFFSET);

	f_mount(&FatFs, "", 0);	            /* Give a work area to the default drive */

	if (f_open(&Fil, "newfile.txt", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
	{	/* Create a file */

		f_write(&Fil, "It works!\r\n", 11, &bw);	/* Write data to the file */

		f_close(&Fil);								/* Close the file */

		printf("It works!\r\n");
	}

	return 0;
}

```

## Internals


## Licence

Copyright (C) 2017, ChaN, all right reserved.

FatFs module is an open source software. Redistribution and use of FatFs in source and binary forms, with or without modification, are permitted provided that the following condition is met:

1. Redistributions of source code must retain the above copyright notice, this condition and the following disclaimer.

This software is provided by the copyright holder and contributors "AS IS" and any warranties related to this software are DISCLAIMED. The copyright owner or contributors be NOT LIABLE for any damages caused by use of this software.
