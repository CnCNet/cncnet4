/*
 * Copyright (c) 2012 Toni Spets <toni.spets@iki.fi>
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

#include "resource.h"
#include "http.h"

#ifdef RELEASE
    #define VERSION_URL "http://cncnet.org/files/version.txt"
#else
    #define VERSION_URL "http://cncnet.org/files/version-dev.txt"
#endif

static DWORD update_check(HWND hwnd)
{
    int ret = IDD_WAIT;
    char buf[256];

    http_init();

    if (http_download_mem(VERSION_URL, buf, sizeof buf))
    {
        if (strncmp(CNCNET_VERSION, buf, strlen(CNCNET_VERSION)) != 0)
        {
            ret = IDD_DOWNLOAD;
        }
    }

    http_release();

    PostMessage(hwnd, WM_USER+2, ret, 0);

    return 0;
}

INT_PTR CALLBACK update_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:

            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), "small"));
            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), "large"));

            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)update_check, hwnd, 0, NULL);

            return TRUE;

        case WM_USER+2:

            EndDialog(hwnd, wParam);

            return TRUE;
    }

    return FALSE;
}
