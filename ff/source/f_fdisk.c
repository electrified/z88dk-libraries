/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT Filesystem Module  R0.13p2                             /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 2017, ChaN, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:
/
/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/
/----------------------------------------------------------------------------*/

#include "ff.h"         /* FatFs Public API */
#include "ffprivate.h"  /* FatFs Private Functions */
#include "ffunicode.h"  /* FatFS Unicode */

#include "__ffstore.h"          /* extern for system storage */
#include "__ffunicodestore.h"   /* extern for LFN system storage */


/*-----------------------------------------------------------------------*/
/* Create Partition Table on the Physical Drive                          */
/*-----------------------------------------------------------------------*/
#if FF_USE_MKFS && !FF_FS_READONLY
#if FF_MULTI_PARTITION

FRESULT f_fdisk (
    BYTE pdrv,            /* Physical drive number */
    const DWORD* szt,    /* Pointer to the size table for each partitions */
    void* work            /* Pointer to the working buffer */
)
{
    UINT i, n, sz_cyl, tot_cyl, b_cyl, e_cyl, p_cyl;
    BYTE s_hd, e_hd, *p, *buf = (BYTE*)work;
    DSTATUS stat;
    DWORD sz_disk, sz_part, s_part;


    stat = disk_initialize(pdrv);
    if (stat & STA_NOINIT) return FR_NOT_READY;
    if (stat & STA_PROTECT) return FR_WRITE_PROTECTED;
    if (disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_disk)) return FR_DISK_ERR;

    /* Determine the CHS without any consideration of the drive geometry */
    for (n = 16; n < 256 && sz_disk / n / 63 > 1024; n *= 2) ;
    if (n == 256) n--;
    e_hd = n - 1;
    sz_cyl = 63 * n;
    tot_cyl = sz_disk / sz_cyl;

    /* Create partition table */
    MEMSET(buf, 0, FF_MAX_SS);
    p = buf + MBR_Table; b_cyl = 0;
    for (i = 0; i < 4; i++, p += SZ_PTE) {
        p_cyl = (szt[i] <= 100U) ? (DWORD)tot_cyl * szt[i] / 100 : szt[i] / sz_cyl;    /* Number of cylinders */
        if (p_cyl == 0) continue;
        s_part = (DWORD)sz_cyl * b_cyl;
        sz_part = (DWORD)sz_cyl * p_cyl;
        if (i == 0) {    /* Exclude first track of cylinder 0 */
            s_hd = 1;
            s_part += 63; sz_part -= 63;
        } else {
            s_hd = 0;
        }
        e_cyl = b_cyl + p_cyl - 1;    /* End cylinder */
        if (e_cyl >= tot_cyl) return FR_INVALID_PARAMETER;

        /* Set partition table */
        p[1] = s_hd;                        /* Start head */
        p[2] = (BYTE)(((b_cyl >> 2) & 0xC0) | 1);    /* Start sector and cylinder high */
        p[3] = (BYTE)b_cyl;                    /* Start cylinder low */
        p[4] = 0x07;                        /* System type (temporary setting) */
        p[5] = e_hd;                        /* End head */
        p[6] = (BYTE)(((e_cyl >> 2) & 0xC0) | 63);    /* End sector and cylinder high */
        p[7] = (BYTE)e_cyl;                    /* End cylinder low */
        st_dword(p + 8, s_part);            /* Start sector in LBA */
        st_dword(p + 12, sz_part);            /* Number of sectors */

        /* Next partition */
        b_cyl += p_cyl;
    }
    st_word(p, 0xAA55);

    /* Write it to the MBR */
    return (disk_write(pdrv, buf, 0, 1) != RES_OK || disk_ioctl(pdrv, CTRL_SYNC, 0) != RES_OK) ? FR_DISK_ERR : FR_OK;
}

#endif /* FF_MULTI_PARTITION */
#endif /* FF_USE_MKFS && !FF_FS_READONLY */

