/*
 * Copyright (c) 2010, 2011 Toni Spets <toni.spets@iki.fi>
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

BOOL FileExists(const char *path)
{
    FILE* file;
    if( (file = fopen(path, "r")) )
    {
        fclose(file);
        return TRUE;
    }
    return FALSE;
}

// replacement for dirname() POSIX function (also keeps internal copy of the path)
char *GetDirectory(const char *path)
{
    static char buf[MAX_PATH];
    char *ptr;
    strncpy(buf, path, MAX_PATH);
    ptr = strrchr(buf, '\\');
    if(ptr)
    {
        *(ptr+1) = 0;
        return buf;
    }

    return NULL;
}

char *GetFile(const char *path)
{
    static char buf[MAX_PATH];
    char *ptr;
    strncpy(buf, path, MAX_PATH);
    ptr = strrchr(buf, '\\');
    if(ptr)
    {
        return (ptr+1);
    }

    return buf;
}

int protocol_exists();
int protocol_register(const char *name, const char *exe);
int protocol_unregister(const char *name);

int main(int argc, char **argv)
{
    char *protocol = NULL;
    char *exe = NULL;
    char *dll = "wsock32.dll";
    char path[MAX_PATH];

    GetModuleFileName(NULL, path, MAX_PATH);

    char *dir = GetDirectory(path);
    if (dir)
    {
        SetCurrentDirectoryA(dir);
    }

    if (FileExists("RA95.DAT"))
    {
        exe = "RA95.DAT";
        protocol = "ra95";
    }
    else if (FileExists("RA95.EXE"))
    {
        exe = "RA95.EXE";
        protocol = "ra95";
    }
    else if (FileExists("C&C95.EXE"))
    {
        exe = "C&C95.EXE";
        protocol = "cnc95";
        dll = "thipx32.dll";
    }
    else if (FileExists("DTA.DAT"))
    {
        exe = "DTA.DAT";
        protocol = "ts";
    }
    else if (FileExists("SUN.EXE"))
    {
        exe = "GAME.EXE";
        protocol = "ts";
    }
    else if (FileExists("RA2.EXE"))
    {
        exe = "GAME.EXE";
        protocol = "ra2";
    }

    snprintf(path, MAX_PATH-1, "%s%s", dir, exe);

    if (argc < 2)
    {
        if (protocol)
        {
            printf("Protocol is %s://, handler at %s\n", protocol, path);
            if (protocol_register(protocol, path))
            {
                if (FileExists("cncnet.dll"))
                {
                    if (FileExists(dll))
                    {
                        DeleteFile(dll);
                    }
                    MoveFile("cncnet.dll", dll);
                }
                sprintf(path, "%s:// registered successfully!\n", protocol);
                MessageBoxA(NULL, path, "CnCNet", MB_OK);
            }
            else
            {
                sprintf(path, "%s:// register failed :-(\n", protocol);
                MessageBoxA(NULL, path, "CnCNet", MB_OK|MB_ICONERROR);
            }
        }
        else
        {
            MessageBoxA(NULL, "Couldn't detect game, can't guess protocol handler", "CnCNet", MB_OK|MB_ICONERROR);
        }
        return 0;
    }

    return 1;
}

int protocol_exists(const char *name)
{
    HKEY hKey;
    return (RegOpenKey(HKEY_CLASSES_ROOT, name, &hKey) == ERROR_SUCCESS);
}

int protocol_register(const char *name, const char *exe)
{
    HKEY hKey;
    char buf[MAX_PATH];

    if (protocol_exists(name))
    {
        protocol_unregister(name);
    }

    sprintf(buf, "%s\\shell\\open\\command", name);

    if (RegCreateKey(HKEY_CLASSES_ROOT, buf, &hKey) == ERROR_SUCCESS)
    {
        char path[MAX_PATH];
        snprintf(path, MAX_PATH-1, "\"%s\" -LAN %%1", exe);
        RegSetValue(HKEY_CLASSES_ROOT, name, REG_SZ, "URL:CnCNet Launcher Protocol", 0);

        RegOpenKey(HKEY_CLASSES_ROOT, name, &hKey);
        RegSetValueEx(hKey, "URL Protocol", 0, REG_SZ, (const BYTE *)"", 1);

        return (RegSetValue(HKEY_CLASSES_ROOT, buf, REG_SZ, path, 0) == ERROR_SUCCESS);
    }

    return TRUE;
}

int protocol_unregister(const char *name)
{
    char buf[256];

    if (!protocol_exists(name))
    {
        return TRUE;
    }

    sprintf(buf, "%s\\shell\\open\\command", name);
    RegDeleteKey(HKEY_CLASSES_ROOT, buf);
    sprintf(buf, "%s\\shell\\open", name);
    RegDeleteKey(HKEY_CLASSES_ROOT, buf);
    sprintf(buf, "%s\\shell", name);
    RegDeleteKey(HKEY_CLASSES_ROOT, buf);
    return (RegDeleteKey(HKEY_CLASSES_ROOT, name) == ERROR_SUCCESS);
}
