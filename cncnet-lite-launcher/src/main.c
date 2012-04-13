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
#include "../res/resource.h"
#include "http.h"
#include "register.h"
#include "base32.h"
#include <commctrl.h>
#include <winsock2.h>
#include "xz/xz.h"

HWND hwnd_status;
HWND hwnd_settings;
HWND itm_settings;
HWND itm_ok;
HWND itm_cancel;
HANDLE open_settings;

HWND itm_host;
HWND itm_port;
HWND itm_p2p;
HWND itm_exe;
HWND itm_args;

void config_load();
void config_save();

/* guessed from game directory */
char *game = NULL;

extern char cfg_host[512];
extern int cfg_port;
extern char cfg_p2p[32];
extern char cfg_exe[64];
extern char cfg_args[512];
extern int cfg_timeout;
extern char cfg_autoupdate[32];
extern char cfg_extractdll[32];
int only_settings = 0;

bool cncnet_parse_response(char *response, char *url, int *interval)
{
    return sscanf(response, "\"%512[^\"]\",%d", url, interval) == 2;
}

char *cncnet_build_request(const char *path, const char *method, const char *game, int port)
{
    static char request[512];
    snprintf(request, sizeof(request), "%s?type=dumb&method=%s&params[]=%s&params[]=%d", path, method, game, port);
    return request;
}

DWORD WINAPI cncnet_connect(int ctx)
{
    HWND itm_status = GetDlgItem(hwnd_status, IDC_STATUS);
    int i;
    char tmp[512] = { 0 };
    PROCESS_INFORMATION pInfo;
    STARTUPINFOA sInfo;

    SetWindowText(itm_settings, "&Settings");

    if (!only_settings)
    {
        ShowWindow(hwnd_status, SW_SHOW);
        SetForegroundWindow(hwnd_status);
    }

    /* give time to open settings */
    if (WaitForSingleObject(open_settings, 0) != WAIT_OBJECT_0 || only_settings)
    {
        if (!only_settings)
        {
            for (i = cfg_timeout; i > 0; i--)
            {
                char buf[128];
                sprintf(buf, "Connecting to CnCNet in %d seconds...", i);
                SetWindowText(itm_status, buf);

                if (WaitForSingleObject(open_settings, 1000) == WAIT_OBJECT_0)
                {
                    break;
                }
            }

            EnableWindow(itm_settings, FALSE);
        }

        if (WaitForSingleObject(open_settings, 0) == WAIT_OBJECT_0)
        {
            ShowWindow(hwnd_status, SW_HIDE);
            ShowWindow(hwnd_settings, SW_SHOW);
            SetForegroundWindow(hwnd_settings);
            return 0;
        }
    }

    SetWindowText(itm_status, "Connecting to CnCNet...");

    /* launch gaem */
    ZeroMemory(&sInfo, sizeof(STARTUPINFO));
    sInfo.cb = sizeof(sInfo);
    ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));

    /* settings passed to dll */
    SetEnvironmentVariable("CNCNET_HOST", cfg_host);
    snprintf(tmp, sizeof(tmp), "%d", cfg_port);
    SetEnvironmentVariable("CNCNET_PORT", tmp);
    SetEnvironmentVariable("CNCNET_P2P", cfg_p2p);

    snprintf(tmp, sizeof(tmp), "%s %s", cfg_exe, cfg_args);
    if (CreateProcess(NULL, tmp, NULL, NULL, TRUE, 0, NULL, NULL, &sInfo, &pInfo) != 0)
    {
        exit(0);
    }
    else
    {
        snprintf(tmp, sizeof(tmp), "Failed to launch %s", cfg_exe);
        SetWindowText(itm_status, tmp);
    }

    return 0;
}

DWORD WINAPI cncnet_download(int ctx)
{
    HWND itm_status = GetDlgItem(hwnd_status, IDC_STATUS);
    char buf[MAX_PATH] = { 0 };
    char *file;
    PROCESS_INFORMATION pInfo;
    STARTUPINFOA sInfo;

    if (http_download_mem("http://cncnet.org/files/version.txt", buf, sizeof(buf)))
    {
        if (strncmp(CNCNET_VERSION, buf, strlen(CNCNET_VERSION)) != 0)
        {
            SetWindowText(itm_status, "Downloading update...");
            if (http_download_file("http://cncnet.org/files/cncnet.exe", "cncnet.tmp"))
            {
                GetModuleFileName(NULL, buf, MAX_PATH);
                file = GetFile(buf);
                rename(file, "cncnet.ex_");
                rename("cncnet.tmp", file);
                SetWindowText(itm_status, "Restarting...");
                ZeroMemory(&sInfo, sizeof(STARTUPINFO));
                sInfo.cb = sizeof(sInfo);
                ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));
                if (CreateProcess(NULL, file, NULL, NULL, TRUE, 0, NULL, NULL, &sInfo, &pInfo) != 0)
                {
                    exit(0);
                }
                else
                {
                    SetWindowText(itm_status, "Error restarting :-(");
                }
            }
            else
            {
                SetWindowText(itm_status, "Error downloading update :-(");
            }
        }
        else
        {
            SetWindowText(itm_status, "No updates found.");
        }
    }

    SetEvent(open_settings);
    return 0;
}

DWORD WINAPI cncnet_update(int ctx)
{
    HWND itm_status = GetDlgItem(hwnd_status, IDC_STATUS);
    HANDLE thread;

    SetWindowText(itm_settings, "&Cancel");
    SetWindowText(itm_status, "Checking for updates...");

    ShowWindow(hwnd_status, SW_SHOW);
    SetForegroundWindow(hwnd_status);

    thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cncnet_download, 0, 0, NULL);

    WaitForSingleObject(open_settings, INFINITE);

    Sleep(1000);

    ResetEvent(open_settings);
    cncnet_connect(ctx);

    return 0;
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        return TRUE;
    }

    if (uMsg == WM_COMMAND && (lParam == 0 || lParam == (LPARAM)itm_cancel))
    {
        PostQuitMessage(0);
    }

    if (uMsg == WM_COMMAND && lParam == (LPARAM)itm_settings)
    {
        SetEvent(open_settings);
    } 

    if (uMsg == WM_COMMAND && lParam == (LPARAM)itm_ok)
    {
        ShowWindow(hwnd, SW_HIDE);
        config_save();
        if (!only_settings)
        {
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cncnet_connect, 0, 0, NULL);
        }
        else
        {
            PostQuitMessage(0);
        }
    }

    return FALSE;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    char path[MAX_PATH];
    char *dir, *exe = NULL, *dll = "wsock32.dll", *dl_ = "wsock32.dl_";
    WIN32_FIND_DATA file;
    HANDLE find;
    BOOL ret;
    MSG msg;
    HRSRC hResInfo;

    MessageBox(NULL, "This is a beta release, it will not auto-update by default and might have problems connecting. Use at your own risk.", "CnCNet", MB_OK|MB_ICONEXCLAMATION);

    http_init();

    hwnd_status = CreateDialog(NULL, MAKEINTRESOURCE(IDD_CONNECTING), NULL, DialogProc);
    hwnd_settings = CreateDialog(NULL, MAKEINTRESOURCE(IDD_SETTINGS), NULL, DialogProc);
    itm_settings = GetDlgItem(hwnd_status, IDC_SETTINGS);
    itm_host = GetDlgItem(hwnd_settings, IDC_HOST);
    itm_port = GetDlgItem(hwnd_settings, IDC_PORT);
    itm_p2p = GetDlgItem(hwnd_settings, IDC_P2P);
    itm_exe = GetDlgItem(hwnd_settings, IDC_EXECUTABLE);
    itm_args = GetDlgItem(hwnd_settings, IDC_ARGUMENTS);
    itm_ok = GetDlgItem(hwnd_settings, IDOK);
    itm_cancel = GetDlgItem(hwnd_settings, IDCANCEL);

    open_settings = CreateEvent(NULL, TRUE, FALSE, NULL);

    SendMessage(hwnd_status, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(hInstance, "small"));
    SendMessage(hwnd_status, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(hInstance, "large"));
    SendMessage(hwnd_settings, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(hInstance, "small"));
    SendMessage(hwnd_settings, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(hInstance, "large"));

    /* set initial focus */
    SendMessage(hwnd_status, WM_NEXTDLGCTL, (WPARAM)itm_settings, TRUE);
    SendMessage(hwnd_settings, WM_NEXTDLGCTL, (WPARAM)itm_ok, TRUE);

    /* fix working directory */
    GetModuleFileName(NULL, path, MAX_PATH);
    dir = GetDirectory(path);
    if (dir)
    {
        SetCurrentDirectoryA(dir);
    }

    /* make a good guess which game we are going to run */
    if (FileExists("C&C95.EXE"))
    {
        exe = "C&C95.EXE";
        game = "cnc95";
        dll = "thipx32.dll";
        dl_ = "thipx32.dl_";
    }
    else if (FileExists("RA95.DAT"))
    {
        exe = "RA95.DAT";
        game = "ra95";
    }
    else if (FileExists("RA95.EXE"))
    {
        exe = "RA95.EXE";
        game = "ra95";
    }
    else if (FileExists("DTA.DAT"))
    {
        exe = "DTA.DAT";
        game = "tsdta";
    }
    else if (FileExists("SUN.EXE"))
    {
        exe = "SUN.EXE";
        game = "ts";
    }
    else if (FileExists("RA2MD.EXE"))
    {
        exe = "RA2MD.EXE";
        game = "ra2yr";
    }
    else if (FileExists("RA2.EXE"))
    {
        exe = "RA2.EXE";
        game = "ra2";
    }

    config_load();

    if (game == NULL)
    {
        if (strlen(cfg_exe) == 0)
        {
            MessageBox(NULL, "Couldn't find any compatible game in current directory. Sorry :-(", "CnCNet", MB_OK|MB_ICONERROR);
            return 1;
        }

        /* generic game type */
        game = "cncnet";
        exe = cfg_exe;
    }

    if (strlen(cfg_exe) == 0)
    {
        strcpy(cfg_exe, exe);
        SetWindowText(itm_exe, cfg_exe);
    }

    /* populate exe selection */
    find = FindFirstFile("*.EXE", &file);
    if (find)
    {
        do {
            SendMessage(itm_exe, CB_ADDSTRING, 0, (WPARAM)file.cFileName);
        } while (FindNextFile(find, &file));
        FindClose(find);
    }

    snprintf(path, MAX_PATH-1, "%s%s", dir, exe);

    /* makes v3 full work! */
    protocol_register(game, path);


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

    /* extract cncnet.dll */
    if (cfg_extractdll[0] == 't')
    {
        if (FileExists(dll))
        {
            SetFileAttributes(dll, FILE_ATTRIBUTE_NORMAL);
            rename(dll, dl_);
            DeleteFile(dl_);
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

    if (strstr(GetCommandLine(), "-CFG"))
    {
        only_settings = 1;
        SetEvent(open_settings);
    }

    if (cfg_autoupdate[0] == 't' && !only_settings)
    {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cncnet_update, 0, 0, NULL);
    }
    else
    {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cncnet_connect, 0, 0, NULL);
    }

    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) 
    { 
        if (ret == -1)
        {
            break;
        }
        else if ((!IsWindow(hwnd_status) || !IsDialogMessage(hwnd_status, &msg)) && (!IsWindow(hwnd_settings) || !IsDialogMessage(hwnd_settings, &msg)))
        { 
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        } 
    } 

    http_release();

    return 0;
}
