/*----------------------------------------------------------------------/
/ Low level disk I/O module function checker                            /
/-----------------------------------------------------------------------/
/ WARNING: The data on the target drive will be lost!
*/

// zcc +yaz180 -subtype=basic_dcio -v --list -m -SO3 -clib=sdcc_iy --max-allocs-per-node100000 diskio_check.c -o diskio_check -create-app

// zcc +rc2014 -subtype=basic_dcio -v --list -m -SO3 -clib=sdcc_iy --max-allocs-per-node100000 diskio_check.c -o diskio_check -create-app

/*
doke &h8124,&h9000




print usr(0)

*/
#include <stdio.h>
#include <string.h>

#if __YAZ180
#include <arch/yaz180/diskio.h> /* Declarations of device I/O functions */
#elif __RC2014
#include <arch/rc2014/diskio.h> /* Declarations of device I/O functions */
#else
#warning "no diskio.h functions available"
#endif

// #include "ffconf.h"     /* Declaration of sector size */
#define FF_MIN_SS		512
#define FF_MAX_SS		512
/* This set of options configures the range of sector size to be supported. (512,
/  1024, 2048 or 4096) Always set both 512 for most systems, generic memory card and
/  harddisk. But a larger value may be required for on-board flash memory and some
/  type of optical media. When FF_MAX_SS is larger than FF_MIN_SS, FatFs is configured
/  for variable sector size mode and disk_ioctl() function needs to implement
/  GET_SECTOR_SIZE command. */


DWORD buffer[1024];  /* 4096 byte working buffer */

static
DWORD pn (		/* Pseudo random number generator */
    DWORD pns	/* 0:Initialise, !0:Read */
)
{
    static DWORD lfsr;
    UINT n;


    if (pns) {
        lfsr = pns;
        for (n = 0; n < 32; n++) pn(0);
    }
    if (lfsr & 1) {
        lfsr >>= 1;
        lfsr ^= 0x80200003;
    } else {
        lfsr >>= 1;
    }
    return lfsr;
}


int test_diskio (
    BYTE pdrv,      /* Physical drive number to be checked (all data on the drive will be lost) */
    // UINT ncyc,      /* Number of test cycles */
    DWORD* buff,    /* Pointer to the working buffer */
    UINT sz_buff    /* Size of the working buffer in unit of byte */
)
{
    UINT n, ns;
    DWORD sz_drv, lba, lba2,  pns = 1;
    WORD sz_eblk, sz_sect;
    BYTE *pbuff = (BYTE*)buff;
    DSTATUS ds;
    DRESULT dr;

    BYTE current_byte = 0;
    BYTE retries = 0;
    BYTE success = 0;
    UINT bytes_in_error = 0;

    printf("test_diskio(%u, %u, 0x%04X)\n", pdrv, (UINT)buff, sz_buff);

    if (sz_buff < FF_MAX_SS + 4) {
        printf("Insufficient work area to run program.\n");
        return 1;
    }

    /* Initialization */
    printf(" disk_initalize(%u)", pdrv);
    ds = disk_initialize(pdrv);
    if (ds & STA_NOINIT) {
        printf(" - failed.\n");
        return 2;
    } else {
        printf(" - ok.\n");
    }

    /* Get drive serial */
    printf("**** Get drive serial number ****\n");
    printf(" disk_ioctl(%u, ATA_GET_SN, 0x%04X)", pdrv, pbuff);
    dr = disk_ioctl(pdrv, ATA_GET_SN, pbuff);

    if (dr == RES_OK) {
        printf(" - ok.\n");
    } else {
        printf(" - failed %u.\n", dr);
    }

    printf("Serial number of the drive %u is %s.\n", pdrv, pbuff);

    /* Get drive size */
    printf("**** Get drive size ****\n");
    printf(" disk_ioctl(%u, GET_SECTOR_COUNT, 0x%04X)", pdrv, (DWORD)&sz_drv);
    sz_drv = 0;
    dr = disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_drv);

    if (dr == RES_OK) {
        printf(" - ok.\n");
    } else {
        printf(" - failed %u.\n", dr);
        return 3;
    }

    printf("Number of sectors on the drive %u is %lu.\n", pdrv, sz_drv);

    if (sz_drv < 128) {
        printf("Failed: Insufficient drive size to test.\n");
        return 4;
    }

    /* Get sector size */
    printf("**** Get sector size ****\n");
    printf(" disk_ioctl(%u, GET_SECTOR_SIZE, 0x%X)", pdrv, (WORD)&sz_sect);
    sz_sect = 0;
    dr = disk_ioctl(pdrv, GET_SECTOR_SIZE, &sz_sect);
    if (dr == RES_OK) {
        printf(" - ok.\n");
    } else {
        printf(" - failed %u.\n", dr);
        return 5;
    }
    printf(" Size of sector is %u bytes.\n", sz_sect);

    /* Get erase block size */
    printf("**** Get block size ****\n");
    printf(" disk_ioctl(%u, GET_BLOCK_SIZE, 0x%X)", pdrv, (WORD)&sz_eblk);
    sz_eblk = 0;
    dr = disk_ioctl(pdrv, GET_BLOCK_SIZE, &sz_eblk);
    if (dr == RES_OK) {
        printf(" - ok.\n");
    } else {
        printf(" - failed %u.\n", dr);
    }
    if (dr == RES_OK || sz_eblk >= 2) {
        printf(" Size of the erase block is %u sectors.\n", sz_eblk);
    } else {
        printf(" Size of the erase block is unknown.\n");
    }

    while (1) {
        // printf("**** Test cycle %u of %u start ****\n", cc, ncyc);

        /* Multiple sector write test */
        printf("**** Multiple sector write test ****\n");
        lba = 1; ns = sz_buff / sz_sect;
        if (ns > 4) ns = 4;
        for (n = 0, pn(pns); n < (UINT)(sz_sect * ns); n++) pbuff[n] = (BYTE)pn(0);
        printf(" disk_write(%u, 0x%X, %lu, %u)", pdrv, (UINT)pbuff, lba, ns);
        dr = disk_write(pdrv, pbuff, lba, ns);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 11;
        }
        printf(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
        dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 12;
        }
        memset(pbuff, 0, sz_sect * ns);

        success = 0;
        retries = 0;
        while (retries < 10 && !success ) {
          bytes_in_error = 0;
          printf(" disk_read(%u, 0x%X, %lu, %u)", pdrv, (UINT)pbuff, lba, ns);
          dr = disk_read(pdrv, pbuff, lba, ns);
          if (dr == RES_OK) {
              printf(" - ok.\n");
          } else {
              printf(" - failed.\n");
              return 13;
          }

          for (n = 0, pn(pns); n < (UINT)(sz_sect * ns); n++){
            current_byte = (BYTE)pn(0);
             if (pbuff[n] != current_byte) {
              //  printf("Expected: 0x%X actual: 0x%X\n", current_byte, pbuff[n]);
               bytes_in_error++;
             }
          }

         if (bytes_in_error == 0) {
             printf(" Data matched.\n");
             success = 1;
         } else {
             printf("Failed: Read data differs from the data written. in error: %u pns: %u\n", bytes_in_error, pns);
         }

          retries++;
        }
        pns++;

        printf("**** Test cycle completed ****\n\n");
        // getchar();
    }

    return 0;
}

int main (void)
{
    test_diskio(0, buffer, sizeof( buffer) );
    return 0;
}
