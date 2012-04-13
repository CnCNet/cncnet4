/*
 * Copyright (c) 2011 Toni Spets <toni.spets@iki.fi>
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

#ifdef WIN32
    #include <winsock.h>
    #include <winsock2.h>
    #include <wsipx.h>
    typedef int socklen_t;
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

#include <stdint.h>
#include <unistd.h>

#define NET_BUF_SIZE 2048

int net_reuse(uint16_t sock);
int net_address(struct sockaddr_in *addr, const char *host, uint16_t port);
void net_address_ex(struct sockaddr_in *addr, uint32_t ip, uint16_t port);

int net_init();
void net_free();

int net_bind(const char *ip, int port);

uint32_t net_read_size();
int8_t net_read_int8();
int16_t net_read_int16();
int32_t net_read_int32();
int net_read_data(void *, size_t);
int net_read_string(char *str, size_t len);

int net_write_int8(int8_t);
int net_write_int16(int16_t);
int net_write_int32(int32_t);
int net_write_data(void *, size_t);
int net_write_string(char *str);
int net_write_string_int32(int32_t);

int net_recv(struct sockaddr_in *);
int net_send(struct sockaddr_in *);
int net_send_noflush(struct sockaddr_in *dst);
void net_send_discard();
void net_broadcast(int from);

void net_peer_remove(uint8_t index);
void net_peer_remove_by_addr(struct sockaddr_in *peer);
uint8_t net_peer_count();
void net_peer_reset();
uint8_t net_peer_get_by_addr(struct sockaddr_in *peer);
uint8_t net_peer_add(struct sockaddr_in *peer);
struct sockaddr_in *net_peer_get(uint8_t index);
intptr_t *net_peer_data(uint8_t index);

extern int net_socket;
extern int net_open;
