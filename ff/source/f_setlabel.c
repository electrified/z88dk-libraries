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
/* Set Volume Label                                                      */
/*-----------------------------------------------------------------------*/
#if FF_USE_LABEL
#if !FF_FS_READONLY

FRESULT f_setlabel (
    const TCHAR* label    /* Pointer to the volume label to set */
)
{
    FRESULT res;
    DIR dj;
    FATFS *fs;
    BYTE dirvn[22];
    UINT i, j, slen;
    WCHAR w;
    static const char badchr[] = "\"*+,.:;<=>\?[]|\x7F";


    /* Get logical drive */
    res = find_volume(&label, &fs, FA_WRITE);
    if (res != FR_OK) LEAVE_FF(fs, res);
    dj.obj.fs = fs;

    /* Get length of given volume label */
    for (slen = 0; (UINT)label[slen] >= ' '; slen++) {}    /* Get name length */

#if FF_FS_EXFAT
    if (fs->fs_type == FS_EXFAT) {    /* On the exFAT volume */
        for (i = j = 0; i < slen; ) {    /* Create volume label in directory form */
            w = label[i++];
#if !FF_LFN_UNICODE    /* ANSI/OEM API */
            if (dbc_1st((BYTE)w)) {
                w = (i < slen && dbc_2nd((BYTE)label[i])) ? w << 8 | (BYTE)label[i++] : 0;
            }
            w = ff_oem2uni(w, CODEPAGE);
#endif
            if (w == 0 || chk_chr(badchr, w) || j == 22) {    /* Check validity check validity of the volume label */
                LEAVE_FF(fs, FR_INVALID_NAME);
            }
            st_word(dirvn + j, w); j += 2;
        }
        slen = j;
    } else
#endif
    {    /* On the FAT/FAT32 volume */
        for ( ; slen && label[slen - 1] == ' '; slen--) ;    /* Remove trailing spaces */
        if (slen != 0) {        /* Is there a volume label to be set? */
            dirvn[0] = 0; i = j = 0;    /* Create volume label in directory form */
            do {
#if FF_LFN_UNICODE && FF_USE_LFN    /* Unicode API */
                w = ff_uni2oem(ff_wtoupper(label[i++]), CODEPAGE);
#else                                /* ANSI/OEM API */
                w = (BYTE)label[i++];
                if (dbc_1st((BYTE)w)) {
                    w = (j < 10 && i < slen && dbc_2nd((BYTE)label[i])) ? w << 8 | (BYTE)label[i++] : 0;
                }
#if FF_USE_LFN
                w = ff_uni2oem(ff_wtoupper(ff_oem2uni(w, CODEPAGE)), CODEPAGE);
#else
                if (IsLower(w)) w -= 0x20;            /* To upper ASCII characters */
#if FF_CODE_PAGE == 0
                if (ExCvt && w >= 0x80) w = ExCvt[w - 0x80];    /* To upper extended characters (SBCS cfg) */
#elif FF_CODE_PAGE < 900
                if (w >= 0x80) w = ExCvt[w - 0x80];    /* To upper extended characters (SBCS cfg) */
#endif
#endif
#endif
                if (w == 0 || chk_chr(badchr, w) || j >= (UINT)((w >= 0x100) ? 10 : 11)) {    /* Reject invalid characters for volume label */
                    LEAVE_FF(fs, FR_INVALID_NAME);
                }
                if (w >= 0x100) dirvn[j++] = (BYTE)(w >> 8);
                dirvn[j++] = (BYTE)w;
            } while (i < slen);
            while (j < 11) dirvn[j++] = ' ';    /* Fill remaining name field */
            if (dirvn[0] == DDEM) LEAVE_FF(fs, FR_INVALID_NAME);    /* Reject illegal name (heading DDEM) */
        }
    }

    /* Set volume label */
    dj.obj.sclust = 0;        /* Open root directory */
    res = dir_sdi(&dj, 0);
    if (res == FR_OK) {
        res = dir_read(&dj, 1);    /* Get volume label entry */
        if (res == FR_OK) {
            if (FF_FS_EXFAT && fs->fs_type == FS_EXFAT) {
                dj.dir[XDIR_NumLabel] = (BYTE)(slen / 2);    /* Change the volume label */
                MEMCPY(dj.dir + XDIR_Label, dirvn, slen);
            } else {
                if (slen != 0) {
                    MEMCPY(dj.dir, dirvn, 11);    /* Change the volume label */
                } else {
                    dj.dir[DIR_Name] = DDEM;    /* Remove the volume label */
                }
            }
            fs->wflag = 1;
            res = sync_fs(fs);
        } else {            /* No volume label entry or an error */
            if (res == FR_NO_FILE) {
                res = FR_OK;
                if (slen != 0) {    /* Create a volume label entry */
                    res = dir_alloc(&dj, 1);    /* Allocate an entry */
                    if (res == FR_OK) {
                        MEMSET(dj.dir, 0, SZDIRE);    /* Clear the entry */
                        if (FF_FS_EXFAT && fs->fs_type == FS_EXFAT) {
                            dj.dir[XDIR_Type] = 0x83;        /* Create 83 entry */
                            dj.dir[XDIR_NumLabel] = (BYTE)(slen / 2);
                            MEMCPY(dj.dir + XDIR_Label, dirvn, slen);
                        } else {
                            dj.dir[DIR_Attr] = AM_VOL;        /* Create volume label entry */
                            MEMCPY(dj.dir, dirvn, 11);
                        }
                        fs->wflag = 1;
                        res = sync_fs(fs);
                    }
                }
            }
        }
    }

    LEAVE_FF(fs, res);
}

#endif /* !FF_FS_READONLY */
#endif /* FF_USE_LABEL */

