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

#include "loader.h"
#include "config.h"

#include <windows.h>
#include <shlwapi.h>

SOCKET WINAPI fake_socket(int af, int type, int protocol);
int WINAPI fake_bind(SOCKET s, const struct sockaddr *name, int namelen);
int WINAPI fake_recvfrom(SOCKET s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen);
int WINAPI fake_sendto(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen);
int WINAPI fake_getsockopt(SOCKET s, int level, int optname, char *optval, int *optlen);
int WINAPI fake_setsockopt(SOCKET s, int level, int optname, const char *optval, int optlen);
int WINAPI fake_getsockname(SOCKET s, struct sockaddr *name, int *namelen);

struct iat_table wsock32_inj[] =
{
    {
        "wsock32.dll",
        {
            { 23,   "socket",       fake_socket },
            { 2,    "bind",         fake_bind },
            { 17,   "recvfrom",     fake_recvfrom },
            { 20,   "sendto",       fake_sendto },
            { 7,    "getsockopt",   fake_getsockopt },
            { 21,   "setsockopt",   fake_setsockopt },
            { 6,    "getsockname",  fake_getsockname },
            { 0, "", NULL }
        }
    },
    {
        "",
        {
            { 0, "", NULL }
        }
    }
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        /* check if we were injected or loaded directly */
        char buf[256];
        GetModuleFileNameA(hinstDLL, buf, 256);

        if (StrStrIA(buf, "wsock32.dll") == NULL)
        {
            loader(wsock32_inj);
        }

#ifdef INTERNET
        config_init();
#endif
    }

    return TRUE;
}
