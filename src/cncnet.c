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

#define STR_TRUE(s) (s != NULL && (s[0] == '1' || s[0] == 't' || s[0] == 'T' || s[0] == 'y' || s[0] == 'Y' || s[0] == 'e' || s[0] == 'E'))

#include "net.h"

#include <windows.h>

#include <stdio.h>
#include <ctype.h>

#include <wsipx.h>
#include <assert.h>
#include <stdio.h>

int my_p2p = 0;

int dedicated = 0;
HMODULE wolapi_dll = NULL;
struct sockaddr_in server;

#define CFG_SECTION "CnCNet4"
#define CFG_PATH    ".\\cncnet.ini"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        char cfg_p2p[6] = "false";
        char cfg_host[256] = "server.cncnet.org";
        int cfg_port = 9001;

        #ifdef _DEBUG
        freopen("stdout.txt", "w", stdout);
        setvbuf(stdout, NULL, _IONBF, 0); 
        #endif

        printf("CnCNet git~%s\n", CNCNET_REV);

        net_init();

        if (getenv("CNCNET_ENABLED"))
        {
            printf("Going into online mode...\n");

            /* allow CnCNet's wolapi.dll to inject itself */
            if (GetFileAttributes("wolapi.dll") != INVALID_FILE_ATTRIBUTES)
            {
                printf("Loading wolapi.dll for WOL mode\n");
                wolapi_dll = LoadLibrary("wolapi.dll");
            }

            GetPrivateProfileString(CFG_SECTION, "P2P", cfg_p2p, cfg_p2p, sizeof cfg_p2p, CFG_PATH);
            GetPrivateProfileString(CFG_SECTION, "Host", cfg_host, cfg_host, sizeof cfg_host, CFG_PATH);
            cfg_port = GetPrivateProfileInt(CFG_SECTION, "Port", cfg_port, CFG_PATH);

            if (STR_TRUE(cfg_p2p))
            {
                printf("Peer-to-peer is enabled\n");
                my_p2p = 1;
            }

            if (cfg_port < 1024 || cfg_port > 65534)
            {
                cfg_port = 9001;
            }

            printf("Broadcasting to %s:%d\n", cfg_host, cfg_port);

            dedicated = 1;
            net_address(&server, cfg_host, cfg_port);

            if (my_p2p)
            {
                net_bind("0.0.0.0", 8054);
            }
        }
        else
        {
            printf("CnCNet: Going into LAN mode...\n");
            net_address(&server, "255.255.255.255", 5000);
            net_bind("0.0.0.0", 5000);
        }
    }

    if (fdwReason == DLL_PROCESS_DETACH)
    {
        net_free();

        if (wolapi_dll)
            FreeLibrary(wolapi_dll);
    }

    return TRUE;
}

SOCKET WINAPI fake_socket(int af, int type, int protocol)
{
#ifdef _DEBUG
    printf("socket(af=%08X, type=%08X, protocol=%08X)\n", af, type, protocol);
#endif

    if (af == AF_IPX)
    {
        return net_socket;
    }

    return socket(af, type, protocol);
}

int WINAPI fake_bind(SOCKET s, const struct sockaddr *name, int namelen)
{
#ifdef _DEBUG
    printf("bind(s=%d, name=%p, namelen=%d)\n", s, name, namelen);
#endif

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

        if (ret > 0)
        {
            if (dedicated)
            {
                if (from_in.sin_addr.s_addr == server.sin_addr.s_addr && from_in.sin_port == server.sin_port)
                {
                    from_in.sin_zero[0] = net_read_int8(); /* p2p flag */
                    from_in.sin_addr.s_addr = net_read_int32();
                    from_in.sin_port = net_read_int16();
                }
                else if (my_p2p)
                {
                    from_in.sin_zero[0] = 1; /* p2p flag for direct packets */
                }
                else
                {
                    /* discard p2p packets if not in p2p mode */
                    return 0;
                }

                /* force p2p port */
                if (from_in.sin_zero[0])
                {
                    from_in.sin_port = htons(8054);
                }

                in2ipx(&from_in, (struct sockaddr_ipx *)from);
            }
            else
            {
                in2ipx(&from_in, (struct sockaddr_ipx *)from);
            }
            ret = net_read_data((void *)buf, len);
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

        if (dedicated)
        {
            if (is_ipx_broadcast((struct sockaddr_ipx *)to))
            {
                net_write_int8(my_p2p ? 1 : 0);
                net_write_int32(0xFFFFFFFF);
                net_write_int16(0xFFFF);
                net_write_data((void *)buf, len);
                net_send(&server);
            }
            else
            {
                ipx2in((struct sockaddr_ipx *)to, &to_in);

                /* use p2p only if both clients are in p2p mode */
                if (to_in.sin_zero[0] && my_p2p) {
                    net_write_data((void *)buf, len);
                    net_send(&to_in);
                } else {
                    net_write_int8(my_p2p ? 1 : 0);
                    net_write_int32(to_in.sin_addr.s_addr);
                    net_write_int16(to_in.sin_port);
                    net_write_data((void *)buf, len);
                    net_send(&server);
                }
            }

            return len;
        }

        ipx2in((struct sockaddr_ipx *)to, &to_in);
        net_write_data((void *)buf, len);

        /* check if it's a broadcast */
        if (is_ipx_broadcast((struct sockaddr_ipx *)to))
        {
            net_send(&server);
            return len;
        }
        else
        {
            return net_send(&to_in);
        }
    }

    return sendto(s, buf, len, flags, to, tolen);
}

int WINAPI fake_getsockopt(SOCKET s, int level, int optname, char *optval, int *optlen)
{
#ifdef _DEBUG
    printf("getsockopt(s=%d, level=%08X, optname=%08X, optval=%p, optlen=%p (%d))\n", s, level, optname, optval, optlen, *optlen);
#endif

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
#ifdef _DEBUG
    printf("setsockopt(s=%d, level=%08X, optname=%08X, optval=%p, optlen=%d)\n", s, level, optname, optval, optlen);
#endif

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
#ifdef _DEBUG
    printf("closesocket(s=%d)\n", s);
#endif

    if (s == net_socket)
    {
        return 0;
    }

    return closesocket(s);
}

int WINAPI fake_getsockname(SOCKET s, struct sockaddr *name, int *namelen)
{
#ifdef _DEBUG
    printf("getsockname(s=%d, name=%p, namelen=%p (%d)\n", s, name, namelen, *namelen);
#endif

    /* this is a hack for Carmageddon LAN, internet play does not work because this is used */
    if (s == net_socket)
    {
        struct sockaddr_in name_in;
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
    }

    return getsockname(s, name, namelen);;
}

int WINAPI _IPX_Initialise()
{
#ifdef _DEBUG
    printf("_IPX_Initialise()\n");
#endif
    return 1;
}

int WINAPI _IPX_Open_Socket95(int s)
{
#ifdef _DEBUG
    printf("_IPX_Open_Socket95(s=%d)\n", s);
#endif
    return 0;
}

int WINAPI _IPX_Start_Listening95()
{
#ifdef _DEBUG
    printf("_IPX_Start_Listening95()\n");
#endif
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
    char *from = ptr + 22;
    char *buf = ptr + 30;

    struct sockaddr_ipx ipx_from;
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
        size_t ipx_len = sizeof (struct sockaddr_ipx);
        ret = fake_recvfrom(net_socket, buf, 900, 0, (struct sockaddr *)&ipx_from, &ipx_len);

        if (ret > 0)
        {
            *len = htons(ret + 30);
            memcpy(from, ipx_from.sa_nodenum, 6);
            *(from + 6) = ipx_from.sa_netnum[1];
            return 1;
        }
    }

    return 0;
}

int WINAPI _IPX_Broadcast_Packet95(void *buf, int len)
{
#ifdef _DEBUG
    printf("_IPX_Broadcast_Packet95(buf=%p, len=%d)\n", buf, len);
#endif
    struct sockaddr_ipx to;
    memset(&to, 0xFF, sizeof (struct sockaddr_ipx));
    return fake_sendto(net_socket, buf, len, 0, (struct sockaddr *)&to, sizeof (struct sockaddr_ipx));
}

int WINAPI _IPX_Send_Packet95(void *ptr, void *buf, int len, void *unk1, void *unk2)
{
#ifdef _DEBUG
    printf("_IPX_Send_Packet95(ptr=%p, buf=%p, len=%d, unk1=%p, unk2=%p)\n", ptr, buf, len, unk1, unk2);
#endif
    struct sockaddr_ipx to;
    memcpy(&to.sa_nodenum, ptr, 6);
    to.sa_netnum[1] = *((char *)ptr + 6);

    return fake_sendto(net_socket, buf, len, 0, (struct sockaddr *)ptr, sizeof (struct sockaddr_ipx));
}

int WINAPI _IPX_Get_Connection_Number95()
{
#ifdef _DEBUG
    printf("_IPX_Get_Connection_Number95()\n");
#endif
    return 0;
}

int WINAPI _IPX_Get_Local_Target95(void *p1, void *p2, void *p3, void *p4)
{
#ifdef _DEBUG
    printf("_IPX_Get_Local_Target95(p1=%p, p2=%p, p3=%p, p4=%p)\n", p1, p2, p3, p4);
#endif
    return 1;
}

int WINAPI _IPX_Close_Socket95(int s)
{
#ifdef _DEBUG
    printf("_IPX_Close_Socket95(s=%d)\n", s);
#endif

    fake_closesocket(net_socket);
    return 0;
}

int WINAPI _IPX_Shut_Down95()
{
#ifdef _DEBUG
    printf("_IPX_Shut_Down95()\n");
#endif
    return 1;
}
