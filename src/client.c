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
#include <strings.h>
#include "resource.h"

typedef struct {
    int idd;
    DLGPROC DialogProc;
} dlg_map;

INT_PTR CALLBACK connect_DialogProc(HWND, UINT, WPARAM, LPARAM);

dlg_map dialogs[] =
{
    { IDD_CONNECT, connect_DialogProc },
};

DLGPROC idd2proc(int idd)
{
    int i;

    for (i = 0; i < sizeof dialogs; i++)
        if (dialogs[i].idd == idd)
            return dialogs[i].DialogProc;

    return NULL;
}

char** ParseCommandLine(char *args, int *nargs)
{
    int len, dq = 0;
    char *l = NULL, *p = args;
    char **list = calloc(256, sizeof(char *));

    *nargs = 0;
    len = strlen(args);

    while (len > 0 && p <= args + len && *nargs < 256)
    {
        if (l && ((!dq && *p == ' ') || (dq && *p == '"') || (p == args + len)))
        {
            list[(*nargs)++] = l;
            l = NULL;
            dq = 0;
            *p = '\0';
        }
        else if (*p == '"')
        {
            dq = !dq;
        }
        else if (l == NULL && (*p != ' ' || dq))
        {
            l = p;
        }

        p++;
    }

    return list;
}

#define FileExists(a) (GetFileAttributes(a) != INVALID_FILE_ATTRIBUTES)

int arg_quiet = 0;
int arg_uninstall = 0;
char *arg_proto = NULL;
char *arg_exe = NULL;
char *arg_url = NULL;

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int next_dialog = IDD_CONNECT;
    char dir[MAX_PATH], path[MAX_PATH], *file;
    char **argv;
    int argc;

    GetModuleFileName(NULL, path, MAX_PATH);

    /* parse arguments */
    argv = ParseCommandLine(lpCmdLine, &argc);

    /* set arguments */
    for (int i = 0; i < argc; i++)
    {
        if (stricmp(argv[i], "/H") == 0)
        {
            printf("usage: %s [/Q] [/U] [exe] \"url\"\r\n", path);
            return 0;
        }

        else if (stricmp(argv[i], "/Q") == 0)
        {
            arg_quiet = 1;
        }

        else if (stricmp(argv[i], "/U") == 0)
        {
            arg_uninstall = 1;
        }

        else if (strnicmp(argv[i], "/P", 2) == 0 && !arg_proto)
        {
            if (strlen(argv[i]) > 2)
            {
                arg_proto = argv[i] + 2;
            }
        }

        else if (strnicmp(argv[i], "/E", 2) == 0 && !arg_exe)
        {
            if (strlen(argv[i]) > 2)
            {
                arg_exe = argv[i] + 2;
            }
        }

        else if (arg_url == NULL)
        {
            arg_url = argv[i];
        }
    }

    free(argv);

    /* fix working directory */
    GetFullPathName(path, sizeof dir, dir, &file);
    if (file)
        *file = '\0';
    SetCurrentDirectoryA(dir);

    /* guess exe if we don't have one */
    if (arg_exe == NULL)
    {
        if (FileExists("C&C95.EXE"))
        {
            arg_exe = "C&C95.EXE";
            arg_proto = "cnc95";
        }
        else if (FileExists("RA95.EXE"))
        {
            arg_exe = "RA95.EXE";
            arg_proto = "ra95";
        }
    }
    else if (!FileExists(arg_exe))
    {
        char strbuf[512];
        snprintf(strbuf, sizeof strbuf, "%s not found in current directory.", arg_exe);
        MessageBox(NULL, strbuf, "CnCNet", MB_OK|MB_ICONERROR);
        return 0;
    }

    if (arg_uninstall && arg_proto)
    {
        HKEY hKey;
        char strbuf[512];

        if (RegOpenKey(HKEY_CLASSES_ROOT, arg_proto, &hKey) != ERROR_SUCCESS)
        {
            snprintf(strbuf, sizeof strbuf, "CnCNet is not registered for %s://.", arg_proto);
            MessageBox(NULL, strbuf, "CnCNet", MB_OK|MB_ICONINFORMATION);
            return 0;
        }

        snprintf(strbuf, sizeof strbuf, "%s\\shell\\open\\command", arg_proto);
        RegDeleteKey(HKEY_CLASSES_ROOT, strbuf);
        snprintf(strbuf, sizeof strbuf, "%s\\shell\\open", arg_proto);
        RegDeleteKey(HKEY_CLASSES_ROOT, strbuf);
        snprintf(strbuf, sizeof strbuf, "%s\\shell", arg_proto);
        RegDeleteKey(HKEY_CLASSES_ROOT, strbuf);

        snprintf(strbuf, sizeof strbuf, "%s", arg_proto);
        if (RegDeleteKey(HKEY_CLASSES_ROOT, strbuf) == ERROR_SUCCESS)
        {
            snprintf(strbuf, sizeof strbuf, "CnCNet is now unregistered for %s://.", arg_proto);
            MessageBox(NULL, strbuf, "CnCNet", MB_OK|MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxA(NULL, "Error unregistering CnCNet, are you an Administrator?", "CnCNet", MB_OK|MB_ICONERROR);
        }

        return 0;
    }

    if (arg_exe == NULL)
    {
        MessageBox(NULL, "No supported game found in current directory.", "CnCNet", MB_OK|MB_ICONERROR);
        return 0;
    }

    if (arg_exe && arg_proto && !arg_url)
    {
        HKEY hKey;
        char strbuf[512];

        snprintf(strbuf, sizeof strbuf, "%s\\shell\\open\\command", arg_proto);
        if (RegCreateKey(HKEY_CLASSES_ROOT, strbuf, &hKey) == ERROR_SUCCESS)
        {
            char buf[MAX_PATH], buf2[MAX_PATH * 2];
            GetModuleFileName(NULL, buf, MAX_PATH);
            snprintf(buf2, sizeof buf2, "\"%s\" /E%s /P%s \"%%1\"", buf, arg_exe, arg_proto);

            snprintf(strbuf, sizeof strbuf, "URL:CnCNet Protocol for %s", arg_proto);
            RegSetValue(HKEY_CLASSES_ROOT, arg_proto, REG_SZ, strbuf, 0);

            RegOpenKey(HKEY_CLASSES_ROOT, arg_proto, &hKey);
            RegSetValueEx(hKey, "URL Protocol", 0, REG_SZ, (const BYTE *)"", 1);

            printf("Registering %s:// -> %s\r\n", arg_proto, buf2);

            snprintf(strbuf, sizeof strbuf, "%s\\shell\\open\\command", arg_proto);
            if (RegSetValue(HKEY_CLASSES_ROOT, strbuf, REG_SZ, buf2, 0) == ERROR_SUCCESS)
            {
                snprintf(strbuf, sizeof strbuf, "CnCNet is now registered for %s://, please go to http://cncnet.org/ to start a match.", arg_proto);
                MessageBox(NULL, strbuf, "CnCNet", MB_OK|MB_ICONINFORMATION);
                return 0;
            }
            else
            {
                MessageBoxA(NULL, "Error registering CnCNet, are you an Administrator?", "CnCNet", MB_OK|MB_ICONERROR);
                return 0;
            }
        }
    }

    while (next_dialog)
    {
        DLGPROC f = idd2proc(next_dialog);
        if (!f) break;
        next_dialog = DialogBox(NULL, MAKEINTRESOURCE(next_dialog), NULL, f);
    }

    return 0;
}
