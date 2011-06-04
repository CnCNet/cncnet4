/*
 * Copyright (c) 2008, 2010, 2011
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

#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct
{
    uint32_t uid;
    uint32_t last_packet;
    struct sockaddr_in addr;
} carmanet_client;

#define MAX_CLIENTS 16
#define IPLIMIT 4
#define SOFT_TIMEOUT 5
#define HARD_TIMEOUT 3600

carmanet_client clients[MAX_CLIENTS];
uint32_t boot = 0;
uint32_t now = 0;
int sock = 0;

void reset_clients()
{
    int i;
    for(i=0;i<MAX_CLIENTS;i++)
    {
        memset(&clients[i], 0, sizeof(carmanet_client));
    }
}

carmanet_client *new_client(struct sockaddr_in *addr)
{
    int i;

    int ip_count = 0;
    int reuse_last_packet = now+1;
    int reuse_index = -1;
    for (i=0;i<MAX_CLIENTS;i++)
    {
        if (clients[i].last_packet && addr->sin_addr.s_addr == clients[i].addr.sin_addr.s_addr)
        {
            if (clients[i].last_packet < reuse_last_packet)
            {
                reuse_last_packet = clients[i].last_packet;
                reuse_index = i;
            }

            ip_count++;

            if (ip_count == IPLIMIT)
            {
                printf("> Reused client %d for %s:%d\n", reuse_index, inet_ntoa(*(struct in_addr *)&addr->sin_addr.s_addr), ntohs(addr->sin_port));
                memcpy(&clients[reuse_index].addr, addr, sizeof(struct sockaddr_in));
                clients[reuse_index].last_packet = now;

                return &clients[reuse_index];
            }
        }
    }

    for(i=0;i<MAX_CLIENTS;i++)
    {
        if(!clients[i].last_packet)
        {
            printf("> Created new client %d at %s:%d (slot %d/%d)\n", i, inet_ntoa(*(struct in_addr *)&addr->sin_addr.s_addr), ntohs(addr->sin_port), ip_count+1, IPLIMIT);
            memcpy(&clients[i].addr, addr, sizeof(struct sockaddr_in));
            clients[i].last_packet = now;

            return &clients[i];
        }
    }
    printf("Error: Max clients (%d) reached!\n", MAX_CLIENTS);
    return NULL;
}

/* return pointer to client struct on success, NULL if MAX_CLIENTS is reached */
carmanet_client *get_client(struct sockaddr_in *addr)
{
    int i;
    for (i=0;i<MAX_CLIENTS;i++)
    {
        if (!memcmp(&clients[i].addr, addr, sizeof(struct sockaddr_in)))
        {
            return &clients[i];
        }
    }

    return new_client(addr);
}

/* broadcasts a packet to all clients in lobby */
void broadcast(carmanet_client *from, char *buf, int len)
{
    char my_buf[1024];

    memcpy(my_buf, &from->addr.sin_addr.s_addr, 4);
    memcpy(my_buf + 4, &from->addr.sin_port, 2);
    memcpy(my_buf + 6, buf, len);

    from->last_packet = now;

    int i;
    for(i=0;i<MAX_CLIENTS;i++)
    {
        if(clients[i].last_packet && &clients[i] != from)
        {
            net_send(sock, my_buf, len + 6, &clients[i].addr);
        }
    }
}

void server_packet(char *buf, int len, struct sockaddr_in *addr)
{
    carmanet_client *cliptr;

    /* force port faking */
    addr->sin_port = htons(NET_PORT);

    cliptr = get_client(addr);

    if(cliptr)
    {
        broadcast(cliptr, buf, len);
    }
}

int main(int argc, char **argv)
{
    struct sockaddr_in listen;

    reset_clients();

    sock = net_socket();
    net_address_ex(&listen, INADDR_ANY, NET_PORT);
    net_bind(sock, &listen);

    char status[256] = { 0 };

    while(1)
    {
        int i;
        struct sockaddr_in addr;
        int players_online = 0;
        int players_idle = 0;
        int len = 0;
        struct timeval tv = { 1, 0 };
        fd_set read_fds;
        char buf[1024];

        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        if (select(sock + 1, &read_fds, NULL, NULL, &tv) > 0)
        {
            len = net_recv(sock, buf, 1024, &addr);
        }

        now = time(NULL);

        /* clear old status line */
        printf("\r");
        for (i=0;i<strlen(status);i++)
            printf(" ");
        printf("\r");
        fflush(NULL);

        if (len)
        {
            server_packet(buf, len, &addr);
        }

        for(i=0;i<MAX_CLIENTS;i++)
        {
            carmanet_client *cli = &clients[i];

            if (cli->last_packet)
            {
                if (cli->last_packet && cli->last_packet < now-HARD_TIMEOUT)
                {
                    memset(&clients[i], 0, sizeof(carmanet_client));
                }
                else if (cli->last_packet && cli->last_packet < now-SOFT_TIMEOUT)
                {
                    players_idle++;
                }
                else
                {
                    players_online++;
                }
            }
        }

        sprintf(status, "CarmaNet [ %d online, %d idle, total %d/%d ]",
                players_online, players_idle, players_online + players_idle, MAX_CLIENTS);

        printf("%s", status);
        fflush(NULL);
    }

    return 0;
}
