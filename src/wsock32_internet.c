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

#include "sockets.h"
#include "config.h"

#include <windows.h>
#include <wsipx.h>
#include <assert.h>
#include <stdio.h>

static struct sockaddr_in my_addr;
static struct sockaddr_in my_server;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        config_init();
    }

    return TRUE;
}

SOCKET WINAPI fake_socket(int af, int type, int protocol)
{
    printf("socket(af=%08X, type=%08X, protocol=%08X)\n", af, type, protocol);

    if (af == AF_IPX)
    {
        SOCKET s = net_socket();
        net_address_ex(&my_addr, INADDR_ANY, config_port);
        net_address(&my_server, config_server, config_server_port);
        net_broadcast(s);
        bind(s, (const struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
        return s;
    }

    return socket(af, type, protocol);
}

int WINAPI fake_bind(SOCKET s, const struct sockaddr *name, int namelen)
{
    printf("bind(s=%d, name=%p, namelen=%d)\n", s, name, namelen);

    if (((struct sockaddr_ipx *)name)->sa_family == AF_IPX)
    {
        return 0;
    }

    return bind(s, name, namelen);
}

int WINAPI fake_recvfrom(SOCKET s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
    char my_buf[1024];
    struct sockaddr_in from_in;

    assert(sizeof(my_buf) > len);

    int ret = net_recv(s, my_buf, len, &from_in);

    if(ret > 0)
    {
        in2ipx(&from_in, (struct sockaddr_ipx *)from);

        if (memcmp(&from_in, &my_server, sizeof(struct sockaddr_in)) == 0)
        {
            printf("broadcast!\n");
            memcpy(&from_in.sin_addr.s_addr, my_buf, 4);
            memcpy(&from_in.sin_port, my_buf + 4, 2);
            memcpy(buf + 6, my_buf, ret - 6);
            return ret - 6;
        }
    }

#ifdef _DEBUG
    printf("recvfrom(s=%d, buf=%p, len=%d, flags=%08X, from=%p, fromlen=%p (%d) -> %d (err: %d)\n", s, buf, len, flags, from, fromlen, *fromlen, ret, WSAGetLastError());
#endif

    return ret;
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
            net_send(s, buf, len, &my_server);
            return len;
        }

        net_send(s, buf, len, &to_in);
        return len;
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

int WINAPI fake_getsockname(SOCKET s, struct sockaddr *name, int *namelen)
{
    struct sockaddr_in name_in;
    int name_in_len = sizeof(struct sockaddr_in);

    printf("getsockname(s=%d, name=%p, namelen=%p (%d)\n", s, name, namelen, *namelen);

    int ret = getsockname(s, (struct sockaddr *)&name_in, &name_in_len);

    if (ret == 0)
    {
#if 0
        /* this doesn't work, we have binded to 0.0.0.0 */
        printf("getsockname: local ip: %s\n", inet_ntoa(name_in.sin_addr));
        in2ipx(&name_in, (struct sockaddr_ipx *)name);
#else
        char hostname[256];
        struct hostent *he;

        gethostname(hostname, 256);
        he = gethostbyname(hostname);

        printf("getsockname: local hostname: %s\n", hostname);

        if (he)
        {
            printf("getsockname: local ip: %s\n", inet_ntoa(*(struct in_addr *)(he->h_addr_list[0])));
            name_in.sin_addr = *(struct in_addr *)(he->h_addr_list[0]);
            in2ipx(&name_in, (struct sockaddr_ipx *)name);
        }
#endif
    }
    return ret;
}
