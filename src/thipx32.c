/*
 * Copyright (c) 2010, 2011, 2012 Toni Spets <toni.spets@iki.fi>
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

#include <windows.h>
#include <winsock2.h>
#include <wsipx.h>
#include <iphlpapi.h>

#ifdef _DEBUG
    #include <stdio.h>
    #define dprintf(...) fprintf(stderr, __VA_ARGS__)
#else
    #define dprintf(...)
#endif

SOCKET WINAPI ipx_socket(int af, int type, int protocol);
int WINAPI ipx_setsockopt(SOCKET s, int level, int optname, const char *optval, int optlen);
int WINAPI ipx_bind(SOCKET s, const struct sockaddr_ipx *name, int namelen);
int WINAPI ipx_sendto(SOCKET s, const char *buf, int len, int flags, const struct sockaddr_ipx *to, int tolen);
int WINAPI ipx_recvfrom(SOCKET s, char *buf, int len, int flags, struct sockaddr_ipx *from, int *fromlen);

static int my_sa_socket = 0;
static int my_socket = 0;

static void ipxdump(const struct sockaddr_ipx *ipx)
{
    dprintf("IPX @ 0x%08X: %02X.%02X.%02X.%02X %02X:%02X:%02X:%02X:%02X:%02X 0x%04X (%d)\n",
            (unsigned int)ipx,
            (unsigned char)ipx->sa_netnum[0], (unsigned char)ipx->sa_netnum[1],
            (unsigned char)ipx->sa_netnum[2], (unsigned char)ipx->sa_netnum[3],
            (unsigned char)ipx->sa_nodenum[0], (unsigned char)ipx->sa_nodenum[1],
            (unsigned char)ipx->sa_nodenum[2], (unsigned char)ipx->sa_nodenum[3],
            (unsigned char)ipx->sa_nodenum[4], (unsigned char)ipx->sa_nodenum[5],
            ipx->sa_socket, ntohs(ipx->sa_socket));
}

int WINAPI _IPX_Initialise()
{
    dprintf("_IPX_Initialise()\n");
    return 1;
}

int WINAPI _IPX_Open_Socket95(int s)
{
    dprintf("_IPX_Open_Socket95(s=%d)\n", s);
    my_sa_socket = htons(s);
    my_socket = ipx_socket(AF_IPX, 0, 0);
    char enable = 1;
    ipx_setsockopt(my_socket, SOL_SOCKET, SO_BROADCAST, &enable, 1);
    return 0;
}

int WINAPI _IPX_Start_Listening95()
{
    dprintf("_IPX_Start_Listening95()\n");

    struct sockaddr_ipx ipx;
    memset(&ipx, 0, sizeof ipx);

    ipx.sa_family = AF_IPX;
    ipx.sa_socket = my_sa_socket;

    return !ipx_bind(my_socket, &ipx, sizeof ipx);
}

int WINAPI _IPX_Get_Outstanding_Buffer95(char *ptr)
{
    /* didn't bother to look up what kind of stuff is in the header so just nullin' it */
    memset(ptr, 0, 30);

    /* using some old magic */
    short *len = (short *)(ptr + 2);
    char *from = ptr + 22;
    char *buf = ptr + 30;

    struct sockaddr_ipx ipx_from;
    struct timeval tv;
    int ret;

    fd_set read_fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&read_fds);
    FD_SET(my_socket, &read_fds);

    select(my_socket+1, &read_fds, NULL, NULL, &tv);

    if (FD_ISSET(my_socket, &read_fds))
    {
        int ipx_len = sizeof (struct sockaddr_ipx);
        ret = ipx_recvfrom(my_socket, buf, 900, 0, &ipx_from, &ipx_len);

        if (ret > 0)
        {
            *len = htons(ret + 30);
            memcpy(from, ipx_from.sa_nodenum, 6);
            dprintf("_IPX_Get_Outstanding_Buffer95(ptr=%p) -> %d bytes\n", ptr, ret);
            ipxdump(&ipx_from);
            return 1;
        }
    }

    return 0;
}

int WINAPI _IPX_Broadcast_Packet95(void *buf, int len)
{
    dprintf("_IPX_Broadcast_Packet95(buf=%p, len=%d)\n", buf, len);

    struct sockaddr_ipx to;
    memset(&to, 0, sizeof to);
    to.sa_family = AF_IPX;
    memset(&to.sa_nodenum, 0xFF, 6);
    to.sa_socket = my_sa_socket;

    return ipx_sendto(my_socket, buf, len, 0, &to, sizeof to);
}

int WINAPI _IPX_Send_Packet95(void *ptr, void *buf, int len, char *unk1, void *unk2)
{
    dprintf("_IPX_Send_Packet95(ptr=%p, buf=%p, len=%d, unk1=%p, unk2=%p)\n", ptr, buf, len, unk1, unk2);

    struct sockaddr_ipx to;
    memset(&to, 0, sizeof to);
    to.sa_family = AF_IPX;
    memcpy(&to.sa_nodenum, ptr, 6);
    to.sa_socket = my_sa_socket;

    return ipx_sendto(my_socket, buf, len, 0, &to, sizeof to);
}

int WINAPI _IPX_Get_Connection_Number95()
{
    dprintf("_IPX_Get_Connection_Number95()\n");
    return 0;
}

int WINAPI _IPX_Get_Local_Target95(void *p1, void *p2, void *p3, void *p4)
{
    dprintf("_IPX_Get_Local_Target95(p1=%p, p2=%p, p3=%p, p4=%p)\n", p1, p2, p3, p4);
    return 1;
}

int WINAPI _IPX_Close_Socket95(int s)
{
    dprintf("_IPX_Close_Socket95(s=%d)\n", s);
    closesocket(my_socket);
    my_sa_socket = 0;
    my_socket = 0;
    return 0;
}

int WINAPI _IPX_Shut_Down95()
{
    dprintf("_IPX_Shut_Down95()\n");
    return 1;
}
