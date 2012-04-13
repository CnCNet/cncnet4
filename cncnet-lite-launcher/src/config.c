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
#include <ctype.h>
#include "../res/resource.h"

char cfg_host[512] = "server.cncnet.org";
char cfg_p2p[32] = "false";
int cfg_port = 9001;
char cfg_exe[64] = { 0 };
char cfg_args[512] = "-LAN";
int cfg_timeout = 3;
char cfg_autoupdate[32] = "false";
char cfg_extractdll[32] = "true";

extern char *game;

extern HWND itm_settings;
extern HWND itm_host;
extern HWND itm_port;
extern HWND itm_p2p;
extern HWND itm_exe;
extern HWND itm_args;

#define SECTION "CnCNet4"
#define CONFIG ".\\cncnet.ini"

void config_load()
{
    char buf[512];

    GetPrivateProfileString(SECTION, "Host", cfg_host, cfg_host, sizeof(cfg_host), CONFIG);
    cfg_port = GetPrivateProfileInt(SECTION, "Port", cfg_port, CONFIG);
    GetPrivateProfileString(SECTION, "P2P", cfg_p2p, cfg_p2p, sizeof(cfg_p2p), CONFIG);
    GetPrivateProfileString(SECTION, "Executable", cfg_exe, cfg_exe, sizeof(cfg_exe), CONFIG);
    GetPrivateProfileString(SECTION, "Arguments", cfg_args, cfg_args, sizeof(cfg_args), CONFIG);
    cfg_timeout = GetPrivateProfileInt(SECTION, "Timeout", cfg_timeout, CONFIG);
    GetPrivateProfileString(SECTION, "AutoUpdate", cfg_autoupdate, cfg_autoupdate, sizeof(cfg_autoupdate), CONFIG);
    GetPrivateProfileString(SECTION, "ExtractDll", cfg_extractdll, cfg_extractdll, sizeof(cfg_extractdll), CONFIG);

    if (cfg_timeout < 0 || cfg_timeout > 30)
    {
        cfg_timeout = 0;
    }

    if (tolower(cfg_p2p[0]) == 't' || tolower(cfg_p2p[0]) == 'y' || cfg_p2p[0] == '1')
    {
        PostMessage(itm_p2p, BM_SETCHECK, BST_CHECKED, 0);
    }

    SetWindowText(itm_host, cfg_host);
    sprintf(buf, "%d", cfg_port);
    SetWindowText(itm_port, buf); 
    SetWindowText(itm_exe, cfg_exe);
    SetWindowText(itm_args, cfg_args); 
}

void config_save()
{
    char buf[512];

    GetWindowText(itm_host, cfg_host, sizeof(cfg_host));
    WritePrivateProfileString(SECTION, "Host", cfg_host, CONFIG);

    GetWindowText(itm_port, buf, sizeof(buf));
    WritePrivateProfileString(SECTION, "Port", buf, CONFIG);

    if (SendMessage(itm_p2p, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        strcpy(cfg_p2p, "true");
    }
    else
    {
        strcpy(cfg_p2p, "false");
    }
    WritePrivateProfileString(SECTION, "P2P", cfg_p2p, CONFIG);

    GetWindowText(itm_exe, cfg_exe, sizeof(cfg_exe));
    WritePrivateProfileString(SECTION, "Executable", cfg_exe, CONFIG);

    GetWindowText(itm_args, cfg_args, sizeof(cfg_args));
    WritePrivateProfileString(SECTION, "Arguments", cfg_args, CONFIG);

    sprintf(buf, "%d", cfg_timeout);
    WritePrivateProfileString(SECTION, "Timeout", buf, CONFIG);

    WritePrivateProfileString(SECTION, "AutoUpdate", cfg_autoupdate, CONFIG);
    WritePrivateProfileString(SECTION, "ExtractDll", cfg_extractdll, CONFIG);
}
