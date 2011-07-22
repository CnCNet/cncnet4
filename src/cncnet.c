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
#include "net.h"

#include <windows.h>
#include <shlwapi.h>

#include <stdio.h>
#include <ctype.h>

#include <wsipx.h>
#include <assert.h>
#include <stdio.h>

struct sockaddr_in my_addr;

SOCKET WINAPI fake_socket(int af, int type, int protocol);
int WINAPI fake_bind(SOCKET s, const struct sockaddr *name, int namelen);
int WINAPI fake_recvfrom(SOCKET s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen);
int WINAPI fake_sendto(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen);
int WINAPI fake_getsockopt(SOCKET s, int level, int optname, char *optval, int *optlen);
int WINAPI fake_setsockopt(SOCKET s, int level, int optname, const char *optval, int optlen);
int WINAPI fake_closesocket(SOCKET s);

struct iat_table cncnet_inj[] =
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
            { 3,    "closesocket",  fake_closesocket },
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
        char buf[MAX_PATH];
        GetModuleFileNameA(hinstDLL, buf, sizeof(buf));

        if (StrStrIA(buf, "wsock32.dll") == NULL)
        {
            loader(cncnet_inj);
        }

        net_init();
        net_bind("0.0.0.0");

        strncpy(buf, GetCommandLine(), sizeof(buf));
        printf("cmdline: %s\n", buf);

        /* very crude URI parser */
        int i;
        char *params = strstr(buf, "://");
        if (params)
        {
            params += 3;
            for (i = 0; i < strlen(params); i++)
                if (params[i] == ' ')
                    params[i] = '\0';

            char *addr = strtok(params, ",");
            do
            {
                int16_t port = 8054;
                char *str_port = strstr(params, ":");

                if (str_port)
                {
                    *str_port = '\0';
                    str_port++;
                    port = atoi(str_port);
                }

                net_peer_add(addr, port);
            } while ((addr = strtok(NULL, ",")));
        }
    }

    if (fdwReason == DLL_PROCESS_DETACH)
    {
        net_free();
    }

    return TRUE;
}

SOCKET WINAPI fake_socket(int af, int type, int protocol)
{
    printf("socket(af=%08X, type=%08X, protocol=%08X)\n", af, type, protocol);

    if (af == AF_IPX)
    {
        return net_socket;
    }

    return socket(af, type, protocol);
}

int WINAPI fake_bind(SOCKET s, const struct sockaddr *name, int namelen)
{
    printf("bind(s=%d, name=%p, namelen=%d)\n", s, name, namelen);

    if (s == net_socket)
    {
        return 0;
    }

    return bind(s, name, namelen);
}

int WINAPI fake_recvfrom(SOCKET s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
#ifdef _DEBUG
    printf("recvfrom(s=%d, buf=%p, len=%d, flags=%08X, from=%p, fromlen=%p (%d))\n", s, buf, len, flags, from, fromlen, *fromlen);
#endif

    if (s == net_socket)
    {
        int ret;
        struct sockaddr_in from_in;

        ret = net_recv(&from_in);

        if (ret > 0)
        {
            if (net_read_int8() == CMD_BROADCAST)
            {
                from_in.sin_addr.s_addr = net_read_int32();
                from_in.sin_port = net_read_int16();
            }

            ret = net_read_data((void *)buf, len);
            in2ipx(&from_in, (struct sockaddr_ipx *)from);
        }

        return ret;
    }

    return recvfrom(s, buf, len, flags, from, fromlen);
}

int WINAPI fake_sendto(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
#ifdef _DEBUG
    printf("sendto(s=%d, buf=%p, len=%d, flags=%08X, to=%p, tolen=%d\n", s, buf, len, flags, to, tolen);
#endif

    if (to->sa_family == AF_IPX)
    {
        struct sockaddr_in to_in;

        ipx2in((struct sockaddr_ipx *)to, &to_in);

        /* check if it's a broadcast */
        if (is_ipx_broadcast((struct sockaddr_ipx *)to))
        {
            net_write_int8(CMD_BROADCAST);
            net_write_data((void *)buf, len);
            net_broadcast();
            return len;
        }
        else
        {
            net_write_int8(CMD_DIRECT);
            net_write_data((void *)buf, len);
            net_send(&to_in);
        }
    }

    return sendto(s, buf, len, flags, to, tolen);
}

int WINAPI fake_getsockopt(SOCKET s, int level, int optname, char *optval, int *optlen)
{
    if (level == 0x3E8)
    {
        *optval = 1;
        *optlen = 1;
        return 0;
    }

    if (level == 0xFFFF)
    {
        *optval = 1;
        *optlen = 1;
        return 0;
    }

    return getsockopt(s, level, optname, optval, optlen);
}

int WINAPI fake_setsockopt(SOCKET s, int level, int optname, const char *optval, int optlen)
{
    printf("setsockopt(s=%d, level=%08X, optname=%08X, optval=%p, optlen=%d)\n", s, level, optname, optval, optlen);

    if (level == 0x3E8)
    {
        return 0;
    }
    if (level == 0xFFFF)
    {
        return 0;
    }

    return setsockopt(s, level, optname, optval, optlen);
}

int WINAPI fake_closesocket(SOCKET s)
{
    printf("closesocket(s=%d)\n", s);

    if (s == net_socket)
    {
        return 0;
    }

    return closesocket(s);
}
