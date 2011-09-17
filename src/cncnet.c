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

#include "net.h"
#include "base32.h"
#include "uri.h"

#include <windows.h>

#include <stdio.h>
#include <ctype.h>

#include <wsipx.h>
#include <assert.h>
#include <stdio.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        int my_port = 8054, ret;
        char buf[MAX_PATH];
        uri_handle *uri;
        int peers = 0;

        #ifdef _DEBUG
        freopen("stdout.txt", "w", stdout);
        setvbuf(stdout, NULL, _IONBF, 0); 
        #endif

        printf("CnCNet: Init\n");

        net_init();

        strncpy(buf, GetCommandLine(), sizeof(buf));

        char *struri = strrchr(buf, ' ');
        if (struri)
        {
            struri++;
            uri = uri_parse(struri);
            printf("CnCNet: URI protocol: %s\n", uri->protocol);
            printf("CnCNet: URI v4: %s\n", uri_value(uri, "v4"));

            if (uri_value(uri, "v4"))
            {
                ret = base32_decode(uri_value(uri, "v4"), buf, sizeof(buf));
                if (ret > 0)
                {
                    int i;
                    struct sockaddr_in peer;
                    peer.sin_family = AF_INET;
                    peers = ret / 6;
                    printf("CnCNet: Got %d peers\n", peers);
                    for (i = 0; i < ret / 6; i ++)
                    {
                        memcpy(&peer.sin_addr.s_addr, buf + (i * 6), 4);
                        memcpy(&peer.sin_port, buf + (i * 6) + 4, 2);
                        net_peer_add(&peer);
                    }
                }
            }

            if (uri_value(uri, "port"))
            {
                my_port = atoi(uri_value(uri, "port"));
                printf("CnCNet: Self port is %d\n", my_port);
            }

            if (uri_value(uri, "open"))
            {
                printf("CnCNet: Enabled open mode, also bcasting to LAN port %d\n", my_port);
                net_open = 1;
                net_peer_add_by_host("255.255.255.255", my_port);
            }

            uri_free(uri);
        }

        /* if no peers listed, play a LAN game */
        if (!peers && net_open == 0)
        {
            printf("CnCNet: Enabled LAN mode\n");
            net_open = 2;
            my_port = 5000;
            net_peer_add_by_host("255.255.255.255", my_port);
        }

        net_bind("0.0.0.0", my_port);
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

        /* check if allowed peer */
        if (!net_peer_ok(&from_in))
        {
            return 0;
        }

        if (ret > 0)
        {
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
    printf("sendto(s=%d, buf=%p, len=%d, flags=%08X, to=%p, tolen=%d)\n", s, buf, len, flags, to, tolen);
#endif

    if (to->sa_family == AF_IPX)
    {
        struct sockaddr_in to_in;

        ipx2in((struct sockaddr_ipx *)to, &to_in);

        /* check if it's a broadcast */
        if (is_ipx_broadcast((struct sockaddr_ipx *)to))
        {
            net_write_data((void *)buf, len);
            net_broadcast();
            return len;
        }
        else
        {
            net_write_data((void *)buf, len);
            return net_send(&to_in);
        }
    }

    return sendto(s, buf, len, flags, to, tolen);
}

int WINAPI fake_getsockopt(SOCKET s, int level, int optname, char *optval, int *optlen)
{
    printf("getsockopt(s=%d, level=%08X, optname=%08X, optval=%p, optlen=%p (%d))\n", s, level, optname, optval, optlen, *optlen);

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

int WINAPI _IPX_Initialise()
{
    printf("_IPX_Initialise()\n");
    return 1;
}

int WINAPI _IPX_Open_Socket95(int s)
{
    printf("_IPX_Open_Socket95(s=%d)\n", s);
    return 0;
}

int WINAPI _IPX_Start_Listening95()
{
    printf("_IPX_Start_Listening95()\n");
    return 1;
}

int WINAPI _IPX_Get_Outstanding_Buffer95(void *ptr)
{
#if _DEBUG
    printf("_IPX_Get_Outstanding_Buffer95(ptr=%p)\n", ptr);
#endif

    /* didn't bother to look up what kind of stuff is in the header so just nullin' it */
    memset(ptr, 0, 30);

    /* using some old magic */
    int16_t *len = ptr + 2;
    int *from_ip = ptr + 22;
    int16_t *from_port = ptr + 26;
    char *buf = ptr + 30;

    struct sockaddr_in from;
    struct timeval tv;
    int ret;

    if (!net_socket)
    {
        return 0;
    }

    fd_set read_fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&read_fds);
    FD_SET(net_socket, &read_fds);

    select(net_socket+1, &read_fds, NULL, NULL, &tv);

    if(FD_ISSET(net_socket, &read_fds))
    {
        ret = net_recv(&from);

        /* check if allowed peer */
        if (!net_peer_ok(&from))
        {
            return 0;
        }

        *from_ip = from.sin_addr.s_addr;
        *from_port = from.sin_port;

        if (ret == 0)
        {
            return 0;
        }

        *len = htons(net_read_data(buf, 512) + 30);

        return 1;
    }

    return 0;
}

int WINAPI _IPX_Broadcast_Packet95(void *buf, int len)
{
#ifdef _DEBUG
    printf("_IPX_Broadcast_Packet95(buf=%p, len=%d)\n", buf, len);
#endif
    net_write_data(buf, len);
    return (net_broadcast() > 0);
}

int WINAPI _IPX_Send_Packet95(void *ptr, void *buf, int len, void *unk1, void *unk2)
{
#ifdef _DEBUG
    printf("_IPX_Send_Packet95(ptr=%p, buf=%p, len=%d, unk1=%p, unk2=%p)\n", ptr, buf, len, unk1, unk2);
#endif
    struct sockaddr_in to;
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = *(int32_t *)ptr;
    to.sin_port = *(int16_t *)(ptr + 4);

    net_write_data(buf, len);
    return (net_send(&to) > 0);
}

int WINAPI _IPX_Get_Connection_Number95()
{
    printf("_IPX_Get_Connection_Number95()\n");
    return 0;
}

int WINAPI _IPX_Get_Local_Target95(void *p1, void *p2, void *p3, void *p4)
{
    printf("_IPX_Get_Local_Target95(p1=%p, p2=%p, p3=%p, p4=%p)\n", p1, p2, p3, p4);
    return 1;
}

int WINAPI _IPX_Close_Socket95(int s)
{
    printf("_IPX_Close_Socket95(s=%d)\n", s);
    return 0;
}

int WINAPI _IPX_Shut_Down95()
{
    printf("_IPX_Shut_Down95()\n");
    return 1;
}
