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

#include "net.h"

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

#define STAT_INTERVAL 5

carmanet_client clients[MAX_CLIENTS];
uint32_t boot = 0;
uint32_t now = 0;

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
void broadcast(carmanet_client *from)
{
    int len;
    char buf[1024];

    len = net_read_data((void *)buf, 1024);

    net_write_int8(CMD_BROADCAST);
    net_write_int32(from->addr.sin_addr.s_addr);
    net_write_int16(from->addr.sin_port);
    net_write_data((void *)buf, len);

    from->last_packet = now;

    int i;
    for(i=0;i<MAX_CLIENTS;i++)
    {
        if(clients[i].last_packet && &clients[i] != from)
        {
            net_send_noflush(&clients[i].addr);
        }
    }

    net_send_discard();
}

void whoami(carmanet_client *from)
{
    net_write_int8(CMD_WHOAMI);
    net_write_int32(from->addr.sin_addr.s_addr);
    net_send(&from->addr);
}

void server_packet(struct sockaddr_in *addr)
{
    carmanet_client *cliptr;

    /* force port faking */
    addr->sin_port = htons(8055);

    cliptr = get_client(addr);

    if (cliptr)
    {
        switch (net_read_int8())
        {
            case CMD_BROADCAST:
                broadcast(cliptr);
                break;
            case CMD_WHOAMI:
                whoami(cliptr);
                break;
            default:
                break;
        }
    }
}

int main(int argc, char **argv)
{
    reset_clients();

    net_init("0.0.0.0", 8055); // remote doesn't matter, we won't broadcast
    net_bind("0.0.0.0");

    char status[256] = { 0 };

    uint32_t last_packets = 0;
    uint32_t last_bytes = 0;
    uint32_t last_time = 0;

    uint32_t total_packets = 0;
    uint32_t total_bytes = 0;
    uint32_t bps = 0;
    uint32_t pps = 0;

    while(1)
    {
        int i;
        struct sockaddr_in addr;
        int players_online = 0;
        int players_idle = 0;
        int len = 0;
        struct timeval tv = { 1, 0 };
        fd_set read_fds;

        FD_ZERO(&read_fds);
        FD_SET(net_socket, &read_fds);

        if (select(net_socket + 1, &read_fds, NULL, NULL, &tv) > 0)
        {
            len = net_recv(&addr);
            total_packets++;
            total_bytes += len;
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
            server_packet(&addr);
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

        if (now >= last_time + STAT_INTERVAL)
        {
            int stat_elapsed = now - last_time ;
            pps = (total_packets - last_packets) / stat_elapsed;
            bps = (total_bytes - last_bytes) / stat_elapsed;
            last_packets = total_packets;
            last_bytes = total_bytes;
            last_time = now;
        }

        sprintf(status, "CarmaNet [ %d online, %d idle, total %d/%d ] [ %d p/s, %d kB/s | total: %d p, %d kB ]",
                players_online, players_idle, players_online + players_idle, MAX_CLIENTS, pps, bps / 1024, total_packets, total_bytes / 1024);

        printf("%s", status);
        fflush(NULL);
    }

    return 0;
}
