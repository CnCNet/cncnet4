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

#ifndef SOCKETS_H
#define SOCKETS_H

#ifdef WIN32
    #include <winsock.h>
    #include <winsock2.h>
    #include <wsipx.h>

    void ipx2in(struct sockaddr_ipx *from, struct sockaddr_in *to);
    void in2ipx(struct sockaddr_in *from, struct sockaddr_ipx *to);
    int is_ipx_broadcast(struct sockaddr_ipx *addr);

#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>

    #define TRUE 1
    #define FALSE 0
#endif

#include <stdint.h>

#define NET_PORT 8055

#define net_socket() socket(AF_INET, SOCK_DGRAM, 0)
#define net_send(a, b, c, d) sendto(a, b, c, 0, (struct sockaddr *)d, sizeof(struct sockaddr))
#define net_bind(a, b) bind(a, (struct sockaddr *)b, sizeof(struct sockaddr));

int net_broadcast(uint16_t sock);
int net_reuse(uint16_t sock);
int net_address(struct sockaddr_in *addr, char *host, uint16_t port);
void net_address_ex(struct sockaddr_in *addr, uint32_t ip, uint16_t port);
int net_recv(uint16_t sock, char *buf, uint32_t len, struct sockaddr_in *who);

#endif
