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
#include "config.h"
#include "resource.h"

INT_PTR CALLBACK settings_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND host = GetDlgItem(hwnd, IDC_HOST);
            HWND port = GetDlgItem(hwnd, IDC_PORT);
            HWND p2p = GetDlgItem(hwnd, IDC_P2P);
            HWND executable = GetDlgItem(hwnd, IDC_EXECUTABLE);
            HWND arguments = GetDlgItem(hwnd, IDC_ARGUMENTS);
            HWND ok = GetDlgItem(hwnd, IDOK);

            WIN32_FIND_DATA file;
            HANDLE find;

            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), "small"));
            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), "large"));
            SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)ok, TRUE);

            SetWindowText(host, config_get("Host"));
            SetWindowText(port, config_get("Port"));
            SetWindowText(executable, config_get("Executable"));
            SetWindowText(arguments, config_get("Arguments"));

            if (config_get_bool("P2P"))
            {
                PostMessage(p2p, BM_SETCHECK, BST_CHECKED, 0);
            }
            else
            {
                PostMessage(p2p, BM_SETCHECK, BST_UNCHECKED, 0);
            }

            /* populate exe selection */
            find = FindFirstFile("*.EXE", &file);
            if (find)
            {
                do {
                    SendMessage(executable, CB_ADDSTRING, 0, (WPARAM)file.cFileName);
                } while (FindNextFile(find, &file));
                FindClose(find);
            }

            return FALSE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    HWND host = GetDlgItem(hwnd, IDC_HOST);
                    HWND port = GetDlgItem(hwnd, IDC_PORT);
                    HWND p2p = GetDlgItem(hwnd, IDC_P2P);
                    HWND executable = GetDlgItem(hwnd, IDC_EXECUTABLE);
                    HWND arguments = GetDlgItem(hwnd, IDC_ARGUMENTS);
                    char buf[256];

                    GetWindowText(host, buf, 255);
                    config_set("Host", buf);

                    GetWindowText(port, buf, 255);
                    config_set("Port", buf);

                    GetWindowText(executable, buf, 255);
                    config_set("Executable", buf);

                    GetWindowText(arguments, buf, 255);
                    config_set("Arguments", buf);

                    if (SendMessage(p2p, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        config_set("P2P", "true");
                    }
                    else
                    {
                        config_set("P2P", "false");
                    }

                    config_save();

                    EndDialog(hwnd, IDD_CONNECT);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}
