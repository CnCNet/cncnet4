/*
 * Copyright (c) 2011, 2012 Toni Spets <toni.spets@iki.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <windows.h>
#include <stdio.h>
#include "xz/xz.h"
#include "config.h"
#include "resource.h"

typedef struct {
    int idd;
    DLGPROC DialogProc;
} dlg_map;

INT_PTR CALLBACK update_DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK test_DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK download_DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK wait_DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK connect_DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK settings_DialogProc(HWND, UINT, WPARAM, LPARAM);

dlg_map dialogs[] =
{
    { IDD_UPDATE, update_DialogProc },
    { IDD_TEST, test_DialogProc },
    { IDD_DOWNLOAD, download_DialogProc },
    { IDD_WAIT, wait_DialogProc },
    { IDD_CONNECT, connect_DialogProc },
    { IDD_SETTINGS, settings_DialogProc }
};

DLGPROC idd2proc(int idd)
{
    int i;

    for (i = 0; i < sizeof dialogs; i++)
        if (dialogs[i].idd == idd)
            return dialogs[i].DialogProc;

    return NULL;
}

#define FileExists(a) (GetFileAttributes(a) != INVALID_FILE_ATTRIBUTES)

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int next_dialog = IDD_WAIT;
    char dir[MAX_PATH], path[MAX_PATH], *file;

    /* fix working directory */
    GetModuleFileName(NULL, path, MAX_PATH);
    GetFullPathName(path, sizeof dir, dir, &file);
    if (file)
        *file = '\0';
    SetCurrentDirectoryA(dir);

    config_init();

    if (config_get_bool("AutoUpdate"))
    {
        next_dialog = IDD_UPDATE;
    }

    /* remove temporary files */
    if (FileExists("cncnet.tmp"))
    {
        Sleep(1000);
        SetFileAttributes("cncnet.tmp", FILE_ATTRIBUTE_NORMAL);
        DeleteFile("cncnet.tmp");
        if (FileExists("cncnet.tmp"))
        {
            MessageBox(NULL, "Couldn't remove old temporary file cncnet.tmp. Check your permissions!", "CnCNet", MB_OK|MB_ICONERROR);
        }
    }

    if (FileExists("cncnet.ex_"))
    {
        SetFileAttributes("cncnet.ex_", FILE_ATTRIBUTE_NORMAL);
        DeleteFile("cncnet.ex_");
        if (FileExists("cncnet.ex_"))
        {
            Sleep(1000);
            DeleteFile("cncnet.ex_");
        }
        /* removing might fail on the first run if a race condition is met, just ignore this */
    }

    if (FileExists("cncnet.dl_"))
    {
        SetFileAttributes("cncnet.dl_", FILE_ATTRIBUTE_NORMAL);
        DeleteFile("cncnet.dl_");
        if (FileExists("cncnet.dl_"))
        {
            Sleep(1000);
            DeleteFile("cncnet.dl_");
        }
        /* removing might fail on the first run if a race condition is met, just ignore this */
    }

    if (FileExists("thipx32.dl_"))
    {
        SetFileAttributes("thipx32.dl_", FILE_ATTRIBUTE_NORMAL);
        DeleteFile("thipx32.dl_");
        if (FileExists("thipx32.dl_"))
        {
            MessageBox(NULL, "Couldn't remove old temporary file thipx32.dl_. Please remove it yourself.", "CnCNet", MB_OK|MB_ICONERROR);
        }
    }

    if (FileExists("wsock32.dl_"))
    {
        SetFileAttributes("wsock32.dl_", FILE_ATTRIBUTE_NORMAL);
        DeleteFile("wsock32.dl_");
        if (FileExists("wsock32.dl_"))
        {
            MessageBox(NULL, "Couldn't remove old temporary file wsock32.dl_. Please remove it yourself.", "CnCNet", MB_OK|MB_ICONERROR);
        }
    }

    /* make a good guess which game we are going to run */
    if (FileExists("C&C95.EXE"))
    {
        config_set_default("Executable", "C&C95.EXE");
        config_set_default("Dll", "thipx32.dll");
        config_set_default("Arguments", "-LAN");
    }
    else if (FileExists("RA95.DAT"))
    {
        config_set_default("Executable", "RA95.DAT");
    }
    else if (FileExists("RA95.EXE"))
    {
        config_set_default("Executable", "RA95.EXE");
        config_set_default("Arguments", "-LAN");
    }
    else if (FileExists("DTA.DAT"))
    {
        config_set_default("Executable", "DTA.DAT");
        config_set_default("Arguments", "-LAN");
    }
    else if (FileExists("SUN.EXE"))
    {
        config_set_default("Executable", "SUN.EXE");
        config_set_default("Arguments", "-LAN");
    }
    else if (FileExists("RA2MD.EXE"))
    {
        config_set_default("Executable", "RA2MD.EXE");
    }
    else if (FileExists("RA2.EXE"))
    {
        config_set_default("Executable", "RA2.EXE");
    }

    /* force settings dialog if no executable found */
    if (strlen(config_get("Executable")) == 0)
    {
        next_dialog = IDD_SETTINGS;
    }
    else if (config_get_bool("ExtractDll"))
    {
        HRSRC hResInfo;
        const char *dll = config_get("Dll");
        
        if (FileExists(dll))
        {
            SetFileAttributes(dll, FILE_ATTRIBUTE_NORMAL);
            rename(dll, "cncnet.dl_");
            DeleteFile("cncnet.dl_");
        }

        if (FileExists(dll))
        {
            snprintf(path, sizeof(path), "Couldn't replace %s. Please remove the file from your game directory for auto-update to work.", dll);
            MessageBox(NULL, path, "CnCNet", MB_OK|MB_ICONERROR);
        }

        hResInfo = FindResource(NULL, "dll", RT_RCDATA);
        if (hResInfo)
        {
            DWORD src_len = SizeofResource(NULL, hResInfo);
            HGLOBAL hResData = LoadResource(NULL, hResInfo);
            LPVOID src = LockResource(hResData);
            unsigned char buf[BUFSIZ];
            FILE *fh;
            struct xz_buf dec_buf;
            struct xz_dec *dec = xz_dec_init(XZ_DYNALLOC, 1 << 26);

            xz_crc32_init();

            dec_buf.in = src;
            dec_buf.in_pos = 0;
            dec_buf.in_size = src_len;
            dec_buf.out = buf;
            dec_buf.out_pos = 0;
            dec_buf.out_size = sizeof(buf);

            fh = fopen(dll, "wb");
            if (!fh)
            {
                snprintf(path, sizeof(path), "Couldn't replace %s. Check your permissions!", dll);
                MessageBox(NULL, path, "CnCNet", MB_OK|MB_ICONERROR);
                return 1;
            }

            while (dec_buf.in_pos < src_len)
            {
                int dec_ret = xz_dec_run(dec, &dec_buf);

                fwrite(dec_buf.out, dec_buf.out_pos, 1, fh);
                dec_buf.out_pos = 0;

                if (dec_ret == XZ_DATA_ERROR)
                {
                    MessageBox(NULL, "XZ archive is corrupted! Aborting.", "CnCNet", MB_OK|MB_ICONERROR);
                    return 1;
                }

                if (dec_ret != XZ_OK)
                    break;
            }

            fclose(fh);
        }
        else
        {
            MessageBox(NULL, "No dll included.", "CnCNet", MB_OK|MB_ICONERROR);
            return 1;
        }
    }

    if (strlen(config_get("P2P")) == 0)
    {
        next_dialog = IDD_TEST;
    }

    if (strstr(GetCommandLine(), "-CFG"))
    {
        DialogBox(NULL, MAKEINTRESOURCE(IDD_SETTINGS), NULL, idd2proc(IDD_SETTINGS));
        config_free();
        return 0;
    }

    while (next_dialog)
    {
        DLGPROC f = idd2proc(next_dialog);
        if (!f) break;
        next_dialog = DialogBox(NULL, MAKEINTRESOURCE(next_dialog), NULL, f);
    }

    config_free();

    return 0;
}
