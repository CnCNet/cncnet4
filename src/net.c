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

#include "net.h"
#include "log.h"
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <string.h>

static struct sockaddr_in net_local;
static uint8_t net_ibuf[NET_BUF_SIZE];
static uint8_t net_obuf[NET_BUF_SIZE];
static uint32_t net_ipos;
static uint32_t net_ilen;
static uint32_t net_opos;
int net_socket = 0;

#ifdef WIN32
void ipx2in(struct sockaddr_ipx *from, struct sockaddr_in *to)
{
    to->sin_family = AF_INET;
    memcpy(&to->sin_addr.s_addr, from->sa_nodenum, 4);
    memcpy(&to->sin_port, from->sa_nodenum + 4, 2);
    to->sin_zero[0] = from->sa_netnum[1];
}

void in2ipx(struct sockaddr_in *from, struct sockaddr_ipx *to)
{
    to->sa_family = AF_IPX;
    *(DWORD *)&to->sa_netnum = 1;
    to->sa_netnum[1] = from->sin_zero[0];
    memcpy(to->sa_nodenum, &from->sin_addr.s_addr, 4);
    memcpy(to->sa_nodenum + 4, &from->sin_port, 2);
}

int is_ipx_broadcast(struct sockaddr_ipx *addr)
{
    unsigned char ff[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    if (memcmp(addr->sa_netnum, ff, 4) == 0 || memcmp(addr->sa_nodenum, ff, 6) == 0)
        return TRUE;
    else
        return FALSE;
}
#endif

int net_opt_reuse()
{
    int yes = 1;
    setsockopt(net_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof(yes));
    return yes;
}

int net_opt_broadcast()
{
    int yes = 1;
    setsockopt(net_socket, SOL_SOCKET, SO_BROADCAST, (char *) &yes, sizeof(yes));
    return yes;
}

int net_address(struct sockaddr_in *addr, const char *host, uint16_t port)
{
    struct hostent *hent;
    hent = gethostbyname(host);
    if(!hent)
    {
        return 0;
    }

    net_address_ex(addr, *(int *)hent->h_addr_list[0], port);
    return 1;
}

void net_address_ex(struct sockaddr_in *addr, uint32_t ip, uint16_t port)
{
    int size = sizeof(struct sockaddr_in);
    memset(addr, 0, size);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = ip;
    addr->sin_port = htons(port);
}

int net_init()
{
#ifdef WIN32
    WSADATA wsaData;
#endif

    if (net_socket)
    {
        net_free();
    }

#ifdef WIN32
    WSAStartup(0x0101, &wsaData);
#endif

    net_socket = socket(AF_INET, SOCK_DGRAM, 0);
    net_ipos = 0;
    net_opos = 0;
    return net_socket;
}

void net_free()
{
#ifdef WIN32
    closesocket(net_socket);
    WSACleanup();
#else
    close(net_socket);
#endif
    net_socket = 0;
}

int net_bind(const char *ip, int port)
{
    if (!net_socket)
    {
        return 0;
    }

    net_address(&net_local, ip, port);

    return bind(net_socket, (struct sockaddr *)&net_local, sizeof(net_local));
}

uint32_t net_read_size()
{
    return net_ilen - net_ipos;
}

int8_t net_read_int8()
{
    int8_t tmp;
    if (net_ipos + 1 > net_ilen)
        return 0;
    memcpy(&tmp, net_ibuf + net_ipos, 1);
    net_ipos += 1;
    return tmp;
}

int16_t net_read_int16()
{
    int16_t tmp;
    if (net_ipos + 2 > net_ilen)
        return 0;
    memcpy(&tmp, net_ibuf + net_ipos, 2);
    net_ipos += 2;
    return tmp;
}

int32_t net_read_int32()
{
    int32_t tmp;
    if (net_ipos + 4 > net_ilen)
        return 0;
    memcpy(&tmp, net_ibuf + net_ipos, 4);
    net_ipos += 4;
    return tmp;
}

int net_read_data(void *ptr, size_t len)
{
    if (net_ipos + len > net_ilen)
    {
        len = net_ilen - net_ipos;
    }

    memcpy(ptr, net_ibuf + net_ipos, len);
    net_ipos += len;
    return len;
}

int net_read_string(char *str, size_t len)
{
    int i;
    for (i = net_ipos; i < NET_BUF_SIZE; i++)
        if (net_ibuf[i] == '\0')
            break;

    if (len > i - net_ipos)
    {
        len = i - net_ipos;
    }

    memcpy(str, net_ibuf + net_ipos, len);
    str[len] = '\0';
    net_ipos += len + 1;
    return 0;
}

int net_write_int8(int8_t d)
{
    assert(net_opos + 1 <= NET_BUF_SIZE);
    memcpy(net_obuf + net_opos, &d, 1);
    net_opos += 1;
    return 1;
}

int net_write_int16(int16_t d)
{
    int16_t tmp = d;
    assert(net_opos + 2 <= NET_BUF_SIZE);
    memcpy(net_obuf + net_opos, &tmp, 2);
    net_opos += 2;
    return 1;
}

int net_write_int32(int32_t d)
{
    int32_t tmp = d;
    assert(net_opos + 4 <= NET_BUF_SIZE);
    memcpy(net_obuf + net_opos, &tmp, 4);
    net_opos += 4;
    return 1;
}

int net_write_data(void *ptr, size_t len)
{
    assert(net_opos + len <= NET_BUF_SIZE);
    memcpy(net_obuf + net_opos, ptr, len);
    net_opos += len;
    return 1;
}

int net_write_string(char *str)
{
    assert(net_opos + strlen(str) + 1 <= NET_BUF_SIZE);
    memcpy(net_obuf + net_opos, str, strlen(str) + 1);
    net_opos += strlen(str) + 1;
    return 1;
}

int net_write_string_int32(int32_t d)
{
    char str[32];
    snprintf(str, sizeof(str), "%d", d);
    return net_write_string(str);
}

int net_recv(struct sockaddr_in *src)
{
    socklen_t l = sizeof(struct sockaddr_in);
    net_ipos = 0;
    net_ilen = recvfrom(net_socket, (char *)net_ibuf, NET_BUF_SIZE, 0, (struct sockaddr *)src, &l);
    return net_ilen;
}

int net_send(struct sockaddr_in *dst)
{
    int ret = net_send_noflush(dst);
    net_send_discard();
    return ret;
}

int net_send_noflush(struct sockaddr_in *dst)
{
    int ret = sendto(net_socket, (char *)net_obuf, net_opos, 0, (struct sockaddr *)dst, sizeof(struct sockaddr_in));
    return ret;
}

void net_send_discard()
{
    net_opos = 0;
}
