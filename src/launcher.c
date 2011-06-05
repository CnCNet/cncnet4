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

        #define DLL_NAME "wsock32-internet.dll"

        LoadLibraryFunc = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
        remoteName = (LPVOID)VirtualAllocEx(pInfo.hProcess, NULL, strlen(DLL_NAME), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        WriteProcessMemory(pInfo.hProcess, (LPVOID)remoteName, DLL_NAME, strlen(DLL_NAME), NULL);
        CreateRemoteThread(pInfo.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryFunc, (LPVOID)remoteName, 0, NULL);

        ResumeThread(pInfo.hThread);
        CloseHandle(pInfo.hProcess);
        CloseHandle(pInfo.hThread);
        return 0;
    }
    else
    {
        //MessageBoxA();
        return 1;
    }
}

int main(int argc, char **argv)
{
    return launch("CARM95.EXE", "-hires");
}
