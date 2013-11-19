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
#include <time.h>
#include "config.h"
#include "resource.h"
#include "net.h"

#define FileExists(a) (GetFileAttributes(a) != INVALID_FILE_ATTRIBUTES)

static DWORD connect_check(HWND hwnd)
{
    STARTUPINFO sInfo;
    PROCESS_INFORMATION pInfo;
    int s = net_init();
    char buf[MAX_PATH];

    fd_set rfds;
    struct timeval tv;
    struct sockaddr_in to;
    struct sockaddr_in from;
    int start = time(NULL);
    int alive = 0;
    int p2p = config_get_bool("P2P");

    net_address(&to, config_get("Host"), config_get_int("Port"));
    net_bind("0.0.0.0", 8054);

    if (p2p)
    {
        net_write_int8(CMD_TESTP2P);
        net_write_int32(start);
    }
    else
    {
        net_write_int8(CMD_QUERY);
    }

    while (time(NULL) < start + 5)
    {
        net_send_noflush(&to);

        FD_ZERO(&rfds);
        FD_SET(s, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        if (select(s + 1, &rfds, NULL, NULL, &tv) > -1)
        {
            if (FD_ISSET(s, &rfds))
            {
                net_recv(&from);

                if (p2p)
                {
                    if (net_read_int8() == CMD_TESTP2P && net_read_int32() == start)
                    {
                        alive = 1;
                        break;
                    }
                }
                else
                {
                    if (net_read_int8() == CMD_QUERY)
                    {
                        alive = 1;
                        break;
                    }
                }
            }
        }
    }

    net_free();

    if (!alive)
    {
        if (p2p)
        {
            MessageBox(NULL, "The CnCNet server seems to be down. You are in peer-to-peer mode, is your UDP port 8054 open?\n\nIf this problem persists, try to disable peer-to-peer connection.", "CnCNet - Oh snap!", MB_OK|MB_ICONERROR);
        }
        else
        {
            MessageBox(NULL, "The CnCNet server seems to be down, please try again later.", "CnCNet - Oh snap!", MB_OK|MB_ICONERROR);
        }
        PostMessage(hwnd, WM_USER+2, IDD_SETTINGS, 0);
        return 0;
    }

    /* remove temporary files */
    if (FileExists("cncnet.tmp"))
    {
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

    /* CnCNet 5 nag */
    if (stricmp(config_get("Executable"), "C&C95.exe") == 0 || stricmp(config_get("Executable"), "cnc95.exe") == 0 || stricmp(config_get("Executable"), "ra95.exe") == 0 || stricmp(config_get("Executable"), "ra95.dat") == 0) {
        int ret = MessageBox(NULL, "It appears you are playing a game that is supported on CnCNet 5. The new version of CnCNet has many new features that make playing even more fun than before.\n\nWould you like to update to CnCNet 5?", "CnCNet 5 has been released!", MB_YESNO|MB_ICONINFORMATION);
        if (ret == IDYES) {
            ShellExecute(NULL, "open", "http://cncnet.org/5", NULL, NULL, SW_SHOWNORMAL);
            PostMessage(hwnd, WM_USER+2, 0, 0);
            return 0;
        }
    }

    ZeroMemory(&sInfo, sizeof sInfo);
    sInfo.cb = sizeof sInfo;
    ZeroMemory(&pInfo, sizeof pInfo);

    /* settings passed to dll */
    SetEnvironmentVariable("CNCNET_HOST", config_get("Host"));
    SetEnvironmentVariable("CNCNET_PORT", config_get("Port"));
    SetEnvironmentVariable("CNCNET_P2P", config_get("P2P"));

    snprintf(buf, sizeof buf, "%s %s", config_get("Executable"), config_get("Arguments"));
    if (CreateProcess(NULL, buf, NULL, NULL, TRUE, 0, NULL, NULL, &sInfo, &pInfo) != 0)
    {
        PostMessage(hwnd, WM_USER+2, 0, 0);
    }
    else
    {
        snprintf(buf, sizeof buf, "Failed to launch %s", config_get("Executable"));
        MessageBox(NULL, buf, "CnCNet", MB_OK|MB_ICONERROR);
        PostMessage(hwnd, WM_USER+2, IDD_SETTINGS, 0);
    }

    return 0;
}

INT_PTR CALLBACK connect_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:

            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), "small"));
            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), "large"));

            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)connect_check, hwnd, 0, NULL);

            return TRUE;

        case WM_USER+2:
            EndDialog(hwnd, wParam);
            break;
    }

    return FALSE;
}
