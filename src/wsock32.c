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
#include <windows.h>
#include <wsipx.h>
#include <assert.h>
#include <stdio.h>

struct sockaddr_in my_addr;
struct sockaddr_in my_bcast;

SOCKET WINAPI fake_socket(int af, int type, int protocol)
{
    printf("socket(af=%08X, type=%08X, protocol=%08X)\n", af, type, protocol);

    if (af == AF_IPX)
    {
        SOCKET s = net_socket();
        net_address_ex(&my_addr, INADDR_ANY, 8054);
        net_address_ex(&my_bcast, INADDR_BROADCAST, 8054);
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
    memset(buf, 0, len);
    memset(from, 0, *fromlen);

    int ret = net_recv(s, buf, len, (struct sockaddr_in *)from);

    if(ret > 0)
    {
        ((struct sockaddr_ipx *)from)->sa_family = AF_IPX;
    }

    printf("recvfrom(s=%d, buf=%p, len=%d, flags=%08X, from=%p, fromlen=%p (%d) -> %d (err: %d)\n", s, buf, len, flags, from, fromlen, *fromlen, ret, WSAGetLastError());

    return ret;
}

int WINAPI fake_sendto(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
    printf("sendto(s=%d, buf=%p, len=%d, flags=%08X, to=%p, tolen=%d\n", s, buf, len, flags, to, tolen);

    if (to->sa_family == AF_IPX)
    {
        struct sockaddr_ipx *to_ipx = (struct sockaddr_ipx *)to;
        struct sockaddr_in to_in;

        to_in.sin_family = AF_INET;
        to_in.sin_addr.s_addr = ((struct sockaddr_in *)to)->sin_addr.s_addr;
        to_in.sin_port = ((struct sockaddr_in *)to)->sin_port;

        /* check if it's a broadcast */
        if (to_ipx->sa_netnum[0] == 0 && to_ipx->sa_netnum[1] == 0 && to_ipx->sa_netnum[2] == 0 && to_ipx->sa_netnum[3] == 0)
        {
            net_send(s, buf, len, &my_bcast);
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

int WINAPI fake_ioctlsocket(SOCKET s, long cmd, u_long *argp)
{
    printf("ioctlsocket(s=%d, cmd=%08X, argp=%p)\n", s, (unsigned int)cmd, argp);
    return ioctlsocket(s, cmd, argp);
}
