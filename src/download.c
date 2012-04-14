/*
 * Copyright (c) 2011 Toni Spets <toni.spets@iki.fi>
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
#include <commctrl.h>

#include "resource.h"
#include "http.h"

#ifdef RELEASE
    #define CLIENT_URL "http://cncnet.org/files/cncnet.exe"
#else
    #define CLIENT_URL "http://cncnet.org/files/cncnet-dev.exe"
#endif

struct download_data
{
    HWND progress;
    FILE *fh;
    bool abort;
};

static struct download_data data;

bool download_write(void *buf, size_t size, size_t file_pos, size_t file_size, struct download_data *data)
{
    if (data->abort)
        return false;

    if (data->progress)
    {
        PostMessage(data->progress, PBM_SETPOS, (WPARAM)(((float)file_pos / file_size) * 100), 0);
    }

    return fwrite(buf, size, 1, data->fh) == 1;
}

static DWORD download_thread(HWND hwnd)
{
    char path[MAX_PATH], dir[MAX_PATH], *file;
    GetModuleFileName(NULL, path, MAX_PATH);
    GetFullPathName(path, sizeof dir, dir, &file);

    http_init();

    if (!file)
    {
        MessageBox(NULL, "Error finding my own filename, the heck?", "CnCNet", MB_OK|MB_ICONERROR);
        PostMessage(hwnd, WM_USER+2, 0, 0);
        return 0;
    }

    data.fh = fopen("cncnet.tmp", "wb");

    if (!data.fh)
    {
        MessageBox(NULL, "Error opening cncnet.tmp for writing. Could you please remove it for me?", "CnCNet", MB_OK|MB_ICONERROR);
        PostMessage(hwnd, WM_USER+2, 0, 0);
        return 0;
    }

    data.progress = GetDlgItem(hwnd, IDC_PROGRESS);
    SendMessage(data.progress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    if (http_get(CLIENT_URL, (HTTP_CALLBACK)download_write, &data))
    {
        STARTUPINFO sInfo;
        PROCESS_INFORMATION pInfo;

        fclose(data.fh);

        rename(file, "cncnet.ex_");
        rename("cncnet.tmp", file);

        ZeroMemory(&sInfo, sizeof sInfo);
        sInfo.cb = sizeof sInfo;
        ZeroMemory(&pInfo, sizeof pInfo);
        if (CreateProcess(NULL, file, NULL, NULL, TRUE, 0, NULL, NULL, &sInfo, &pInfo) == 0)
        {
            MessageBox(NULL, "Error restarting, new exe is corrupted? :-(", "CnCNet", MB_OK|MB_ICONERROR);
        }
    }
    else
    {
        fclose(data.fh);
    }

    http_release();

    PostMessage(hwnd, WM_USER+2, 0, 0);

    return 0;
}

INT_PTR CALLBACK download_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:

            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), "small"));
            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), "large"));

            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)download_thread, hwnd, 0, NULL);

            return TRUE;

        case WM_USER+2:

            EndDialog(hwnd, wParam);

            return TRUE;

        case WM_CLOSE:
            data.abort = true;
    }

    return FALSE;
}
