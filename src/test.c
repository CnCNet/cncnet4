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
#include <time.h>
#include <commctrl.h>
#include "config.h"
#include "resource.h"
#include "net.h"

static DWORD test_do(HWND hwnd)
{
    int s = net_init();

    fd_set rfds;
    struct timeval tv;
    struct sockaddr_in to;
    struct sockaddr_in from;
    int start = time(NULL);
    int ret = 0;

    net_address(&to, config_get("Host"), config_get_int("Port"));
    net_bind("0.0.0.0", 8054);

    net_write_int8(CMD_TESTP2P);
    net_write_int32(start);

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

                if (net_read_int8() == CMD_TESTP2P && net_read_int32() == start)
                {
                    ret = 1;
                    break;
                }
            }
        }
    }

    close(s);

    PostMessage(hwnd, WM_USER + 2, ret, 0);
    return 0;
}

INT_PTR CALLBACK test_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:

            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), "small"));
            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), "large"));

            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)test_do, hwnd, 0, NULL);

            return TRUE;

        case WM_USER + 2:

            EndDialog(hwnd, IDD_SETTINGS);

            if (!wParam)
            {
                config_set("P2P", "false");
                MessageBox(NULL, "Your UDP port 8054 seems to be closed. Fast peer-to-peer games are not possible, slower tunneled connection will be used.\n\nFor more information, go to http://cncnet.org/setup-guide", "CnCNet", MB_OK|MB_ICONEXCLAMATION);
            }
            else
            {
                config_set("P2P", "true");
                MessageBox(NULL, "CnCNet has successfully detected that you have faster peer-to-peer connection working!", "CnCNet", MB_OK|MB_ICONINFORMATION);
            }

            return TRUE;
    }

    return FALSE;
}
