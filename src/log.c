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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include "log.h"

static char status_line[256] = { 0 };

int log_printf(const char *fmt, ...)
{
    va_list args;
    char tbuf[256] = { 0 };
    time_t now;
    struct tm *tm;

    now = time(NULL);
    tm = localtime(&now);

    log_status_clear();

    if (tm)
    {
        strftime(tbuf, sizeof(tbuf), "[%c] ", tm);
        fputs(tbuf, stdout);
    }

    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    /* force redrawing status line */
    log_statusf(NULL);

    return 0;
}

int log_statusf(const char *fmt, ...)
{
    va_list args;

    if (fmt != NULL)
    {
        status_line[0] = '\0';
        va_start(args, fmt);
        vsnprintf(status_line, sizeof(status_line), fmt, args);
        va_end(args);
    }

    fprintf(stdout, "\r%s", status_line);
    fflush(stdout);

    return 0;
}

void log_status_clear()
{
    int i;
    /* rewind status line out if any, the portable clear line */
    fputc('\r', stdout);
    for (i = 0; i < strlen(status_line) + 4; i++)
        fputc(' ', stdout);
    fputc('\r', stdout);
}

