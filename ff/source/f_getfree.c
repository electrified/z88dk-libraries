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


/*-----------------------------------------------------------------------*/
/* File/Volume controls                                                  */
/*-----------------------------------------------------------------------*/

#if FF_VOLUMES < 1 || FF_VOLUMES > 10
#error Wrong FF_VOLUMES setting
#endif
extern FATFS *FatFs[];      /* Pointer to the filesystem objects (logical drives) */
extern WORD Fsid;		    /* File system mount ID */

#if FF_FS_RPATH != 0 && FF_VOLUMES >= 2
extern BYTE CurrVol;	    /* Current drive */
#endif

#if FF_FS_LOCK != 0
extern FILESEM Files[];     /* Open object lock semaphores */
#endif


/*-----------------------------------------------------------------------*/
/* LFN/Directory working buffer                                          */
/*-----------------------------------------------------------------------*/

#if FF_USE_LFN == 1         /* LFN enabled with static working buffer */
#if FF_FS_EXFAT
extern BYTE DirBuf[];       /* Directory entry block scratchpad buffer */
#endif
extern WCHAR LfnBuf[];      /* LFN working buffer */
#endif


/*------------------------------------------------------------------------*/
/* Code Conversion Tables                                                 */
/*------------------------------------------------------------------------*/

#if FF_USE_LFN != 0                         /* LFN configurations */

#if FF_CODE_PAGE == 0       /* Run-time code page configuration */
#define CODEPAGE CodePage
extern WORD CodePage;       /* Current code page */
extern const BYTE *ExCvt, *DbcTbl;    /* Pointer to current SBCS up-case table and DBCS code range table below */
extern const BYTE Ct437[];
extern const BYTE Ct720[];
extern const BYTE Ct737[];
extern const BYTE Ct771[];
extern const BYTE Ct775[];
extern const BYTE Ct850[];
extern const BYTE Ct852[];
extern const BYTE Ct855[];
extern const BYTE Ct857[];
extern const BYTE Ct860[];
extern const BYTE Ct861[];
extern const BYTE Ct862[];
extern const BYTE Ct863[];
extern const BYTE Ct864[];
extern const BYTE Ct865[];
extern const BYTE Ct866[];
extern const BYTE Ct869[];
extern const BYTE Dc932[];
extern const BYTE Dc936[];
extern const BYTE Dc949[];
extern const BYTE Dc950[];

extern const WORD cp_code[];
extern const WCHAR *const cp_table[];

#elif FF_CODE_PAGE < 900    /* static code page configuration (SBCS) */
#define CODEPAGE FF_CODE_PAGE
extern const BYTE ExCvt[];

#else                       /* static code page configuration (DBCS) */
#define CODEPAGE FF_CODE_PAGE
extern const BYTE DbcTbl[];

#endif
#endif


/*-----------------------------------------------------------------------*/
/* Get Number of Free Clusters                                           */
/*-----------------------------------------------------------------------*/
#if FF_FS_MINIMIZE == 0
#if !FF_FS_READONLY

FRESULT f_getfree (
    const TCHAR* path,  /* Path name of the logical drive number */
    DWORD* nclst,       /* Pointer to a variable to return number of free clusters */
    FATFS** fatfs       /* Pointer to return pointer to corresponding filesystem object */
)
{
    FRESULT res;
    FATFS *fs;
    DWORD nfree, clst, sect, stat;
    UINT i;
    FFOBJID obj;


    /* Get logical drive */
    res = find_volume(&path, &fs, 0);
    if (res == FR_OK) {
        *fatfs = fs;                /* Return ptr to the fs object */
        /* If free_clst is valid, return it without full FAT scan */
        if (fs->free_clst <= fs->n_fatent - 2) {
            *nclst = fs->free_clst;
        } else {
            /* Scan FAT to obtain number of free clusters */
            nfree = 0;
            if (fs->fs_type == FS_FAT12) {    /* FAT12: Scan bit field FAT entries */
                clst = 2; obj.fs = fs;
                do {
                    stat = get_fat(&obj, clst);
                    if (stat == 0xFFFFFFFF) { res = FR_DISK_ERR; break; }
                    if (stat == 1) { res = FR_INT_ERR; break; }
                    if (stat == 0) nfree++;
                } while (++clst < fs->n_fatent);
            } else {
#if FF_FS_EXFAT
                if (fs->fs_type == FS_EXFAT) {    /* exFAT: Scan allocation bitmap */
                    BYTE bm;
                    UINT b;

                    clst = fs->n_fatent - 2;    /* Number of clusters */
                    sect = fs->database;        /* Assuming bitmap starts at cluster 2 */
                    i = 0;                        /* Offset in the sector */
                    do {    /* Counts numbuer of bits with zero in the bitmap */
                        if (i == 0) {
                            res = move_window(fs, sect++);
                            if (res != FR_OK) break;
                        }
                        for (b = 8, bm = fs->win[i]; b && clst; b--, clst--) {
                            if (!(bm & 1)) nfree++;
                            bm >>= 1;
                        }
                        i = (i + 1) % SS(fs);
                    } while (clst);
                } else
#endif
                {    /* FAT16/32: Scan WORD/DWORD FAT entries */
                    clst = fs->n_fatent;    /* Number of entries */
                    sect = fs->fatbase;        /* Top of the FAT */
                    i = 0;                    /* Offset in the sector */
                    do {    /* Counts numbuer of entries with zero in the FAT */
                        if (i == 0) {
                            res = move_window(fs, sect++);
                            if (res != FR_OK) break;
                        }
                        if (fs->fs_type == FS_FAT16) {
                            if (ld_word(fs->win + i) == 0) nfree++;
                            i += 2;
                        } else {
                            if ((ld_dword(fs->win + i) & 0x0FFFFFFF) == 0) nfree++;
                            i += 4;
                        }
                        i %= SS(fs);
                    } while (--clst);
                }
            }
            *nclst = nfree;            /* Return the free clusters */
            fs->free_clst = nfree;    /* Now free_clst is valid */
            fs->fsi_flag |= 1;        /* FAT32: FSInfo is to be updated */
        }
    }

    LEAVE_FF(fs, res);
}

#endif /* !FF_FS_READONLY */
#endif /* FF_FS_MINIMIZE == 0 */
