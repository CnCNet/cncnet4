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
#include <commctrl.h>

#include "resource.h"

static HANDLE ev;

static DWORD wait_tick(HWND hwnd)
{
    int i;
    char fmt[256];
    char buf[256];

    HWND text = GetDlgItem(hwnd, IDC_STATUS);
    GetWindowText(text, fmt, sizeof fmt);

    for (i = 3; i > 0; i--)
    {
        sprintf(buf, fmt, i);
        SetWindowText(text, buf);
        ShowWindow(hwnd, SW_SHOW);

        if (WaitForSingleObject(ev, 1000) == WAIT_OBJECT_0)
        {
            break;
        }
    }

    if (WaitForSingleObject(ev, 0) != WAIT_OBJECT_0)
    {
        PostMessage(hwnd, WM_USER, 0, 0);
    }

    return 0;
}

INT_PTR CALLBACK wait_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:

            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), "small"));
            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), "large"));

            ev = CreateEvent(NULL, TRUE, FALSE, NULL);

            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wait_tick, hwnd, 0, NULL);

            return TRUE;

        case WM_DESTROY:

            CloseHandle(ev);

            return FALSE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    SetEvent(ev);
                    EndDialog(hwnd, IDD_SETTINGS);
                    return TRUE;

                case IDCANCEL:
                    SetEvent(ev);
                    EndDialog(hwnd, 0);
                    return TRUE;
            }
            break;

        case WM_USER:
            EndDialog(hwnd, IDD_CONNECT);
            return TRUE;
    }

    return FALSE;
}
