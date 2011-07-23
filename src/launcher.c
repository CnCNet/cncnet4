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

HANDLE hProcess = NULL;
HANDLE hThread = NULL;

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

int launch(const char *gameExe, const char *gameParams)
{
    PROCESS_INFORMATION pInfo;
    STARTUPINFOA sInfo;

    char *gamePath = NULL;
    char gameParamsFull[MAX_PATH];

    printf("gameExe: %s, gameParams: %s\n", gameExe, gameParams);

    if (!FileExists(gameExe))
    {
        printf("gameExe not found\n");
        return 1;
    }

    gamePath = GetDirectory(gameExe);
    if(gamePath)
    {
        SetCurrentDirectoryA(gamePath);
    }

    snprintf(gameParamsFull, MAX_PATH, "%s %s", GetFile(gameExe), gameParams);

    ZeroMemory(&sInfo, sizeof(STARTUPINFO));
    sInfo.cb = sizeof(sInfo);
    ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));

    if(CreateProcessA(gameExe, (LPSTR)gameParamsFull, 0, 0, FALSE, CREATE_SUSPENDED, 0, 0, &sInfo, &pInfo))
    {
        LPVOID remoteName, LoadLibraryFunc;

        #define DLL_NAME "cncnet.dll"

        LoadLibraryFunc = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
        remoteName = (LPVOID)VirtualAllocEx(pInfo.hProcess, NULL, strlen(DLL_NAME), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        WriteProcessMemory(pInfo.hProcess, (LPVOID)remoteName, DLL_NAME, strlen(DLL_NAME), NULL);
        HANDLE hThread = CreateRemoteThread(pInfo.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryFunc, (LPVOID)remoteName, 0, NULL);

        WaitForSingleObject(hThread, INFINITE);

        ResumeThread(pInfo.hThread);
        CloseHandle(pInfo.hProcess);
        CloseHandle(pInfo.hThread);
        return 0;
    }
    else
    {
        return 1;
    }
}

int main(int argc, char **argv)
{
    char *protocol = NULL;
    char path[MAX_PATH];

    GetModuleFileName(NULL, path, MAX_PATH);

    char *dir = GetDirectory(path);
    if (dir)
    {
        SetCurrentDirectoryA(dir);
    }

    if (FileExists("RA95.DAT") || FileExists("RA95.EXE"))
    {
        protocol = "ra95";
    }
    else if (FileExists("C&C95.EXE"))
    {
        protocol = "cnc95";
    }

    if (argc < 2)
    {
        if (protocol)
        {
            printf("Protocol is %s://, handler at %s\n", protocol, path);
            if (protocol_register(protocol, path))
            {
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

    snprintf(path, MAX_PATH-1, "-LAN %s", argv[1]);

    if (FileExists("RA95.DAT"))
    {
        return launch("RA95.DAT", path);
    }

    if (FileExists("RA95.EXE"))
    {
        return launch("RA95.EXE", path);
    }

    if (FileExists("C&C95.EXE"))
    {
        return launch("C&C95.EXE", path);
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
        GetModuleFileName(NULL, path+1, MAX_PATH);
        path[0] = '"';
        strcat(path, "\" \"%1\"");
        printf("cncnet-launcher at %s\n", path);
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
