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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "net.h"
#include "log.h"
#include "list.h"

/* mingw supports it and I really want getopt(3) */
#include <unistd.h>
#include <stdlib.h>

enum
{
    GAME_UNKNOWN,
    GAME_CNC95,
    GAME_RA95,
    GAME_TS,
    GAME_TSDTA,
    GAME_TSTI,
    GAME_RA2,
    GAME_LAST
};

const char *game_str(int game)
{
    switch (game)
    {
        case GAME_CNC95:    return "C&C95";
        case GAME_RA95:     return "RA95";
        case GAME_TS:       return "TS";
        case GAME_TSDTA:    return "TSDTA";
        case GAME_TSTI:     return "TSTI";
        case GAME_RA2:      return "RA2";
        default:            return "UNKNOWN";
    }
}

enum
{
    CMD_TUNNEL,     /* 0 */
    CMD_P2P,        /* 1 */
    CMD_DISCONNECT, /* 2 */
    CMD_PING,       /* 3 */
    CMD_QUERY,      /* 4 */
    CMD_TESTP2P     /* 5 */
};

typedef struct Client
{
    struct sockaddr_in  addr;
    uint8_t             p2p;
    uint32_t            last_packet;
    uint32_t            last_ping;
    uint32_t            ping_count;
    uint8_t             game;
    struct Client       *next;
} Client;

typedef struct Config
{
    int32_t             port;
    char                ip[32];
    char                hostname[256];
    int32_t             timeout;
    int32_t             maxclients;
} Config;

int interrupt = 0;
void onsigint(int signum)
{
    log_status_clear();
    printf("Received ^C, exiting...");
    interrupt = 1;
}
void onsigterm(int signum)
{
    log_status_clear();
    printf("Received SIGTERM, exiting...");
    interrupt = 1;
}

int main(int argc, char **argv)
{
    Client *client;
    Client *clients = NULL;
    Config config;

    int s, opt;
    fd_set rfds;
    struct timeval tv;
    struct sockaddr_in peer;
    char buf[NET_BUF_SIZE];
    time_t booted = time(NULL);

    uint32_t last_packets = 0;
    uint32_t last_bytes = 0;
    uint32_t last_time = 0;

    uint32_t total_packets = 0;
    uint32_t total_bytes = 0;
    uint32_t bps = 0;
    uint32_t pps = 0;

    config.port = 9001;
    strcpy(config.ip, "0.0.0.0");
    strcpy(config.hostname, "Unnamed CnCNet 4.0 Server");
    config.timeout = 10;
    config.maxclients = 0;

    while ((opt = getopt(argc, argv, "?hi:n:t:c:l:")) != -1)
    {
        switch (opt)
        {
            case 'i':
                strncpy(config.ip, optarg, sizeof(config.ip)-1);
                break;
            case 'n':
                strncpy(config.hostname, optarg, sizeof(config.hostname)-1);
                break;
            case 't':
                config.timeout = atoi(optarg);
                if (config.timeout < 1)
                {
                    config.timeout = 1;
                }
                else if (config.timeout > 3600)
                {
                    config.timeout = 3600;
                }
                break;
            case 'c':
                config.maxclients = atoi(optarg);
                if (config.maxclients < 0)
                {
                    config.maxclients = 0;
                }
                break;
            case 'h':
            case '?':
            default:
                fprintf(stderr, "Usage: %s [-h?] [-i ip] [-n hostname] [-t timeout] [-c maxclients] [port]\n", argv[0]);
                return 1;
        }
    }

    if (optind < argc)
    {
        config.port = atoi(argv[optind]);
        if (config.port < 1024)
        {
            config.port = 1024;
        }
        else if (config.port > 65535)
        {
            config.port = 65535;
        }
    }

    s = net_init();

    printf("CnCNet 4.0 Server\n");
    printf("=================\n");
    printf("         ip: %s\n", config.ip);
    printf("       port: %d\n", config.port);
    printf("   hostname: %s\n", config.hostname);
    printf("    timeout: %d seconds\n", config.timeout);
    printf(" maxclients: %d\n", config.maxclients);
    printf("    version: %s\n", VERSION);
    printf("\n");

    net_bind(config.ip, config.port);

    FD_ZERO(&rfds);
    FD_SET(s, &rfds);
    memset(&tv, 0, sizeof(tv));

    signal(SIGINT, onsigint);
    signal(SIGTERM, onsigterm);

    while (!interrupt)
    {
        time_t now = time(NULL);
        int num_clients = 0;

        if (now > last_time)
        {
            int stat_elapsed = now - last_time ;
            pps = (total_packets - last_packets) / stat_elapsed;
            bps = (total_bytes - last_bytes) / stat_elapsed;
            last_packets = total_packets;
            last_bytes = total_bytes;
            last_time = now;

            num_clients = 0;
            LIST_FOREACH (clients, client)
            {
                num_clients++;
            }

            log_statusf("%s [ %d/%d | %d p/s, %d kB/s | total: %d p, %d kB ]",
                config.hostname, num_clients, config.maxclients, pps, bps / 1024, total_packets, total_bytes / 1024);
        }

        net_send_discard();
        FD_ZERO(&rfds);
        FD_SET(s, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        if (select(s + 1, &rfds, NULL, NULL, &tv) > -1 && !interrupt)
        {
            now = time(NULL);

            if (FD_ISSET(s, &rfds))
            {
                size_t len = net_recv(&peer);
                uint8_t cmd;

                total_packets++;
                total_bytes += len;

                if (len == 0)
                {
                    continue;
                }

                cmd = net_read_int8();

                if (cmd == CMD_QUERY)
                {
                    /* query responds with the basic server information to display on a server browser */
                    int cnt[GAME_LAST] = { 0, 0, 0, 0, 0, 0, 0 };

                    num_clients = 0;
                    LIST_FOREACH (clients, client)
                    {
                        cnt[client->game]++;
                        num_clients++;
                    }

                    net_write_int8(CMD_QUERY);
                    net_write_string("hostname");
                    net_write_string(config.hostname);
                    net_write_string("clients");
                    net_write_string_int32(num_clients);
                    net_write_string("maxclients");
                    net_write_string_int32(config.maxclients);
                    net_write_string("version");
                    net_write_string(VERSION);
                    net_write_string("uptime");
                    net_write_string_int32(now - booted);
                    net_write_string("unk");
                    net_write_string_int32(cnt[GAME_UNKNOWN]);
                    net_write_string("cnc95");
                    net_write_string_int32(cnt[GAME_CNC95]);
                    net_write_string("ra95");
                    net_write_string_int32(cnt[GAME_RA95]);
                    net_write_string("ts");
                    net_write_string_int32(cnt[GAME_TS]);
                    net_write_string("tsdta");
                    net_write_string_int32(cnt[GAME_TSDTA]);
                    net_write_string("tsti");
                    net_write_string_int32(cnt[GAME_TSTI]);
                    net_write_string("ra2");
                    net_write_string_int32(cnt[GAME_RA2]);

                    net_send(&peer);
                    total_packets++;
                    continue;
                }

                if (cmd == CMD_TESTP2P)
                {
                    net_write_int8(CMD_TESTP2P);
                    net_write_int32(net_read_int32());
                    peer.sin_port = htons(8054);

                    net_send(&peer);
                    total_packets++;
                    continue;
                }

                /* look for our client */
                client = NULL;
                Client *a;
                LIST_FOREACH (clients, a)
                {
                    if (a->addr.sin_addr.s_addr == peer.sin_addr.s_addr && a->addr.sin_port == peer.sin_port)
                    {
                        client = a;
                        break;
                    }
                }

                if (client == NULL)
                {
                    /* ignore disconnect packets swhen not connected */
                    if (cmd == CMD_DISCONNECT)
                    {
                        continue;
                    }

                    /* ignore new clients when hitting the maximum, can't do much more than that */
                    if (config.maxclients > 0 && num_clients == config.maxclients)
                    {
                        continue;
                    }

                    client = LIST_NEW(Client);
                    memcpy(&client->addr, &peer, sizeof peer);
                    LIST_INSERT(clients, client);
                }

                if (cmd == CMD_DISCONNECT)
                {
                    log_printf("%s:%d disconnected\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
                    LIST_REMOVE(clients, client);
                    FREE(client);
                    /* special packet from clients who are closing the socket so we can remove them from the active list before timeout */
                    continue;
                }

                if (cmd == CMD_PING)
                {
                    net_read_int32();
                    client->last_packet = now;
                    client->ping_count = 0;
                    continue;
                }

                uint32_t to_ip = net_read_int32();
                uint16_t to_port = net_read_int16();
                Client *client_to = NULL;
                len = net_read_data(buf, sizeof(buf));

                /* discard invalid destinations */
                if (to_ip == 0 || to_port == 0) {
                    /* if it was a complete stray packet, just ignore the client completely */
                    if (client->game == GAME_UNKNOWN)
                    {
                        LIST_REMOVE(clients, client);
                        FREE(client);
                    }
                    continue;
                }

                /* broadcast */
                if (to_ip == 0xFFFFFFFF)
                {
                    /* try to detect any supported game */
                    if (buf[0] == 0x34 && buf[1] == 0x12)
                    {
                        client->game = GAME_CNC95;
                    }
                    else if (buf[0] == 0x35 && buf[1] == 0x12)
                    {
                        client->game = GAME_RA95;
                    }
                    else if (buf[4] == 0x35 && buf[5] == 0x12)
                    {
                        client->game = GAME_TS;
                    }
                    else if (buf[4] == 0x35 && buf[5] == 0x13)
                    {
                        client->game = GAME_TSDTA;
                    }
                    else if (buf[4] == 0x35 && buf[5] == 0x14)
                    {
                        client->game = GAME_TSTI;
                    }
                    else if (buf[4] == 0x36 && buf[5] == 0x12)
                    {
                        client->game = GAME_RA2;
                    }
                    else
                    {
                        client->game = GAME_UNKNOWN;
                    }

                    client->p2p = (cmd == CMD_P2P);

                    if (client->last_packet == 0)
                    {
                        log_printf("%s:%d connected with %s (%s)\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port), game_str(client->game), cmd == CMD_P2P ? "p2p" : "tun");
                    }

                    /* hack: the motd bot can connect with an empty broadcast without broadcasting anything */
                    if (len)
                    {
                        net_write_int8(cmd);
                        net_write_int32(peer.sin_addr.s_addr);

                        /* fake P2P port, always */
                        if (cmd == CMD_P2P)
                        {
                            net_write_int16(htons(8054));
                        }
                        else
                        {
                            net_write_int16(peer.sin_port);
                        }

                        net_write_data(buf, len);

                        LIST_FOREACH (clients, client_to)
                        {
                            /* hack: sending all broadcasts to unknown clients so the welcome bot gets connects, can also be used to monitor cncnet */
                            if (client_to != client && (client_to->game == client->game || client_to->game == GAME_UNKNOWN))
                            {
                                net_send_noflush(&client_to->addr);
                                total_packets++;
                            }
                        }

                        net_send_discard();
                    }
                }
                else
                /* direct */
                {
                    if (client->last_packet == 0)
                    {
                        log_printf("%s:%d connected with direct packet, possibly a desync\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
                    }

                    Client *tmp;
                    LIST_FOREACH (clients, tmp)
                    {
                        /* hack: if someone from the destination ip is registered as p2p client, ignore destination port */
                        if (tmp->addr.sin_addr.s_addr == to_ip && (tmp->addr.sin_port == to_port || (ntohs(to_port) == 8054 && tmp->p2p)))
                        {
                            client_to = tmp;
                            break;
                        }
                    }

                    if (client_to == NULL)
                    {
                        log_printf("%s:%d tried to send to unknown client %s:%d\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port), inet_ntoa(*(struct in_addr *)&to_ip), ntohs(to_port));
                    }
                    else
                    {
                        net_write_int8(cmd);
                        net_write_int32(peer.sin_addr.s_addr);
                        net_write_int16(peer.sin_port);
                        net_write_data(buf, len);
                        net_send(&client_to->addr);
                        total_packets++;
                    }
                }

                client->last_packet = now;
                client->ping_count = 0;
            }

            /* check for timeouts */
            LIST_FOREACH (clients, client)
            {
                if (now - client->last_packet > config.timeout)
                {
                    if (now - client->last_ping > 5 && client->ping_count > 2)
                    {
                        log_printf("%s:%d timed out\n", inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));
                        LIST_REMOVE(clients, client);
                        FREE(client);
                    }
                    else if (now - client->last_ping > 5)
                    {
                        net_write_int8(CMD_PING);
                        net_write_int32(client->ping_count);
                        net_send(&client->addr);
                        client->last_ping = now;
                        client->ping_count++;
                        total_packets++;
                    }
                }
            }
        }
    }

    printf("\n");

    LIST_FREE(clients);

    net_free();
    return 0;
}
