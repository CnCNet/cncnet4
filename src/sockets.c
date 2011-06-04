/*
 * Copyright (c) 2008, 2010
 *      Toni Spets <toni.spets@iki.fi>
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
#include <winsock2.h>
#include <stdio.h>

void ipx2in(struct sockaddr_ipx *from, struct sockaddr_in *to)
{
    to->sin_family = AF_INET;
    memcpy(&to->sin_addr.s_addr, from->sa_nodenum, 4);
    memcpy(&to->sin_port, from->sa_nodenum + 4, 2);
}

void in2ipx(struct sockaddr_in *from, struct sockaddr_ipx *to)
{
    to->sa_family = AF_IPX;
    *(DWORD *)&to->sa_netnum = 1;
    memcpy(to->sa_nodenum, &from->sin_addr.s_addr, 4);
    memcpy(to->sa_nodenum + 4, &from->sin_port, 2);
    to->sa_socket = from->sin_port;
}

int is_ipx_broadcast(struct sockaddr_ipx *addr)
{
    unsigned char ff[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    if (memcmp(addr->sa_netnum, ff, 4) == 0 || memcmp(addr->sa_nodenum, ff, 6) == 0)
        return TRUE;
    else
        return FALSE;
}

int net_broadcast(uint16_t sock)
{
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *) &yes, sizeof(yes));
    return yes;
}

int net_reuse(uint16_t sock)
{
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof(yes));
    return yes;
}

int net_address(struct sockaddr_in *addr, char *host, uint16_t port)
{
    struct hostent *hent;
    hent = gethostbyname(host);
    if(!hent)
    {
        return FALSE;
    }

    fprintf(stderr, "Server host resolved to '%s'\n", inet_ntoa(*(struct in_addr *)hent->h_addr_list[0]));

    net_address_ex(addr, *(int *)hent->h_addr_list[0], port);
    return TRUE;
}

void net_address_ex(struct sockaddr_in *addr, uint32_t ip, uint16_t port)
{
    int size = sizeof(struct sockaddr_in);
    memset(addr, 0, size);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = ip;
    addr->sin_port = htons(port);
}

int net_recv(uint16_t sock, char *buf, uint32_t len, struct sockaddr_in *who)
{
    int tmp = sizeof(struct sockaddr);
    return recvfrom(sock, (char *)buf, len, 0, (struct sockaddr *)who, &tmp);
}
