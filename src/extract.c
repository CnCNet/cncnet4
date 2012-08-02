/*
 * Copyright (c) 2012 Toni Spets <toni.spets@iki.fi>
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
#include <stdio.h>
#include "xz/xz.h"

int cncnet_extract(const char *data, int len, const char *filename)
{
    char buf[BUFSIZ];
    FILE *fh;
    struct xz_buf dec_buf;
    struct xz_dec *dec = xz_dec_init(XZ_DYNALLOC, 1 << 26);

    xz_crc32_init();

    dec_buf.in = (uint8_t *)data;
    dec_buf.in_pos = 0;
    dec_buf.in_size = len;
    dec_buf.out = (uint8_t *)buf;
    dec_buf.out_pos = 0;
    dec_buf.out_size = sizeof buf;

    fh = fopen(filename, "wb");
    if (!fh)
    {
        snprintf(buf, sizeof buf, "Couldn't extract %s. Check your permissions!", filename);
        MessageBox(NULL, buf, "CnCNet", MB_OK|MB_ICONERROR);
        return 0;
    }

    while (dec_buf.in_pos < len)
    {
        int dec_ret = xz_dec_run(dec, &dec_buf);

        fwrite(dec_buf.out, dec_buf.out_pos, 1, fh);
        dec_buf.out_pos = 0;

        if (dec_ret == XZ_DATA_ERROR)
        {
            MessageBox(NULL, "XZ archive is corrupted! Aborting.", "CnCNet", MB_OK|MB_ICONERROR);
            fclose(fh);
            return 0;
        }

        if (dec_ret != XZ_OK)
            break;
    }

    fclose(fh);
    return 1;
}
