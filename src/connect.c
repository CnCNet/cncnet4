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

#define FileExists(a) (GetFileAttributes(a) != INVALID_FILE_ATTRIBUTES)

char mem_get_int8(char **buf, int *pos, int size)
{
    if (*pos < size)
    {
        char ret = **buf;
        *pos += 1;
        *buf += 1;
        return ret;
    }

    return 0;
}

int mem_get_int32(char **buf, int *pos, int size)
{
    if (*pos + 4 <= size)
    {
        int ret = *((int *)*buf);
        *pos += 4;
        *buf += 4;
        return ret;
    }

    return 0;
}

int mem_get_data(char **buf, char *dst, int len, int *pos, int size)
{
    int ret = min(len, size - *pos);

    memcpy(dst, *buf, ret);

    *buf += ret;
    *pos += ret;

    return ret;
}

extern char *arg_exe;
extern char *arg_url;

static DWORD launch_spawn(HWND hwnd)
{
    size_t size = 1024 * 1024;
    char *buf = malloc(size);
    char strbuf[256];

    char gameid[64] = { 0 };
    char name[32] = { 0 };

    char *p = strstr(arg_url, "//");
    if (p && strlen(p) > 2)
    {
        p += 2;

        char *p2 = strstr(p, "/");
        if (p2)
        {
            *p2 = '\0';
            strncpy(gameid, p, sizeof gameid);
            p2++;
            strncpy(name, p2, sizeof name);
        }
    }

    gameid[sizeof gameid - 1] = '\0';
    name[sizeof name - 1] = '\0';

    if (strlen(gameid) == 0 || strlen(name) == 0)
    {
        MessageBox(NULL, "Malformed URL.", "CnCNet", MB_OK|MB_ICONERROR);
        PostMessage(hwnd, WM_USER+2, 0, 0);
        return 0;
    }

    http_init();

    snprintf(strbuf, sizeof strbuf, "http://5.cncnet.org/spawn/%s/%s", gameid, name);
    printf("Fetching spawn data from %s\r\n", strbuf);

    if (http_download_mem(strbuf, buf, &size))
    {
        STARTUPINFO sInfo;
        PROCESS_INFORMATION pInfo;
        int pos = 0;

        while (pos < size)
        {
            int len,rd;
            char *filebuf, type = mem_get_int8(&buf, &pos, size);

            if (type == 1)
            {
                len = mem_get_int32(&buf, &pos, size);
                filebuf = malloc(len);
                rd = mem_get_data(&buf, filebuf, len, &pos, size);

                if (rd == len)
                {
                    FILE *fh = fopen("spawn.ini", "wb");
                    if (fh)
                    {
                        fwrite(filebuf, len, 1, fh);
                        fclose(fh);
                    }
                }

                free(filebuf);

                printf("Read spawn ini %d/%d\n", rd, len);
            }
            else if (type == 2)
            {
                len = mem_get_int32(&buf, &pos, size);
                filebuf = malloc(len);
                rd = mem_get_data(&buf, filebuf, len, &pos, size);

                if (rd == len)
                {
                    FILE *fh = fopen("maptmp.ini", "wb");
                    if (fh)
                    {
                        fwrite(filebuf, len, 1, fh);
                        fclose(fh);
                    }
                }

                free(filebuf);

                printf("Read map ini %d/%d\n", rd, len);
            }
            else if (type == 3)
            {
                len = mem_get_int32(&buf, &pos, size);
                filebuf = malloc(len);
                rd = mem_get_data(&buf, filebuf, len, &pos, size);

                if (rd == len)
                {
                    FILE *fh = fopen("maptmp.bin", "wb");
                    if (fh)
                    {
                        fwrite(filebuf, len, 1, fh);
                        fclose(fh);
                    }
                }

                free(filebuf);

                printf("Read map bin %d/%d\n", rd, len);
            }
            else
            {
                printf("Invalid type %d\n", type);
                break;
            }
        }

        snprintf(strbuf, sizeof strbuf, "%s -SPAWN", arg_exe);

        ZeroMemory(&sInfo, sizeof sInfo);
        sInfo.cb = sizeof sInfo;
        ZeroMemory(&pInfo, sizeof pInfo);

        if (CreateProcess(NULL, strbuf, NULL, NULL, TRUE, 0, NULL, NULL, &sInfo, &pInfo) != 0)
        {
            ShowWindow(hwnd, SW_HIDE);
            WaitForSingleObject(pInfo.hProcess, INFINITE);
            PostMessage(hwnd, WM_USER+2, 0, 0);
        }
        else
        {
            snprintf(strbuf, sizeof strbuf, "Failed to launch %s", arg_exe);
            MessageBox(NULL, strbuf, "CnCNet", MB_OK|MB_ICONERROR);
        }

    } else {
        snprintf(strbuf, sizeof strbuf, "Connection to CnCNet failed, game was not found.");
        MessageBox(NULL, strbuf, "CnCNet", MB_OK|MB_ICONERROR);
    }

    free(buf);

    /* cleanup */
    DeleteFile("spawn.ini");
    DeleteFile("maptmp.ini");
    DeleteFile("maptmp.bin");

    http_release();

    PostMessage(hwnd, WM_USER+2, 0, 0);

    return 0;
}

INT_PTR CALLBACK connect_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:

            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), "small"));
            SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), "large"));

            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)launch_spawn, hwnd, 0, NULL);

            return TRUE;

        case WM_CLOSE:
        case WM_USER+2:
            EndDialog(hwnd, wParam);
            break;
    }

    return FALSE;
}
