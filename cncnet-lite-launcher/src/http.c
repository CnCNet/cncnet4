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

#include "http.h"

static HINTERNET http_handle = NULL;

bool http_init()
{
    return (http_handle = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0));
}

bool http_release()
{
    return http_handle != NULL ? InternetCloseHandle(http_handle) : true;
}

bool http_get(const char *url, HTTP_CALLBACK cb, void *context)
{
    DWORD size = sizeof(DWORD);
    DWORD zero = 0;
    unsigned int file_pos = 0;
    unsigned int file_size = 0;
    unsigned int http_code = 0;
    HINTERNET req;
    char buf[BUFSIZ];

    if (http_handle == NULL)
    {
        return false;
    }

    req = InternetOpenUrl(http_handle, url, NULL, 0, INTERNET_FLAG_NO_COOKIES|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_RELOAD, 0 /* context */);

    if (req == NULL)
    {
        return false;
    }

    zero = 0;
    HttpQueryInfo(req, HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER, &file_size, &size, &zero);

    zero = 0;
    HttpQueryInfo(req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &http_code, &size, &zero);

    if (http_code == 200)
    {
        while (InternetReadFile(req, buf, sizeof(buf), &size))
        {
            if (size == 0)
            {
                break;
            }

            file_pos += size;

            if (cb)
            {
                if (cb((void *)buf, size, file_pos, file_size, context) == false)
                {
                    break;
                }
            }
        }
    }

    InternetCloseHandle(req);
    return (http_code == 200);
}

bool http_write_mem(void *buf, size_t size, size_t file_pos, size_t file_size, download *dl)
{
    if (file_size > dl->bufsiz || dl->bufpos + size > dl->bufsiz)
    {
        return false;
    }

    memcpy((char *)dl->buf + dl->bufpos, buf, size);
    dl->bufpos += size;
    return true;
}

int http_download_mem(const char *url, void *buf, size_t bufsiz)
{
    static download dl;
    dl.buf = buf;
    dl.bufpos = 0;
    dl.bufsiz = bufsiz;
    return http_get(url, (HTTP_CALLBACK)http_write_mem, &dl);
}

bool http_write_file(void *buf, size_t size, size_t file_pos, size_t file_size, FILE *fh)
{
    return fwrite(buf, size, 1, fh) == 1;
}

int http_download_file(const char *url, const char *path)
{
    FILE *fh = fopen(path, "wb");
    bool success = false;

    if (fh)
    {
        success = http_get(url, (HTTP_CALLBACK)http_write_file, fh);
        fclose(fh);
    }

    return success;
}
