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

#include "uri.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* strdup wasn't ANSI and I was very bitchy about being compliant */
char *uri_strdup(const char *str)
{
    char *t;
    t = malloc(strlen(str) + 1);
    strcpy(t, str);
    return t;
}

int uri_add_param(uri_handle *uri, const char *key, const char *value)
{
    uri_param *param = calloc(sizeof(uri_param), 1);
    param->key = uri_strdup(key);
    param->value = value && strlen(value) ? uri_strdup(value) : NULL;

    uri->num_params++;
    uri->params = realloc(uri->params, sizeof(uri_param *) * uri->num_params);
    uri->params[uri->num_params - 1] = param;

    return 1;
}

uri_handle *uri_parse(const char *str)
{
    int i, len = strlen(str);
    uri_handle *uri = calloc(sizeof(uri_handle), 1);
    char *buf = uri_strdup(str), *key = NULL, *value = NULL;

    for (i = 0; i < len; i++)
    {
        if (uri->protocol == NULL)
        {
            if (buf[i] == ':')
            {
                buf[i] = '\0';
                uri->protocol = uri_strdup(buf);
                key = value = NULL;
                if (i < len-1)
                {
                    key = buf+(i+1);
                }
            }
        }
        else
        {
            if (buf[i] == '?')
            {
                key = value = NULL;
                if (i < len-1)
                {
                    key = value = buf+(i+1);
                }
                continue;
            }
            
            if (buf[i] == '=')
            {
                buf[i] = '\0';
                value = NULL;

                if (i < len-1)
                {
                    value = buf+(i+1);
                }
            }

            if (buf[i] == '&')
            {
                buf[i] = '\0';

                if (key && strlen(key))
                {
                    uri_add_param(uri, key, value);
                }

                key = value = NULL;

                if (i < len-1)
                {
                    key = value = buf+(i+1);
                }
            }
        }
    }

    if (key && strlen(key))
    {
        uri_add_param(uri, key, value);
    }

    free(buf);

    return uri;
}

const char *uri_value(uri_handle *uri, const char *key)
{
    int i;

    for (i = 0; i < uri->num_params; i++)
    {
        if (strcmp(uri->params[i]->key, key) == 0)
        {
            return uri->params[i]->value;
        }
    }

    return NULL;
}

int uri_free(uri_handle *uri)
{
    int i;

    if (uri == NULL)
        return 1;

    if (uri->protocol)
    {
        free(uri->protocol);
    }

    for (i = 0; i < uri->num_params; i++)
    {
        if (uri->params[i]->key)
        {
            free(uri->params[i]->key);        
        }

        if (uri->params[i]->value)
        {
            free(uri->params[i]->value);        
        }

        free(uri->params[i]);
    }

    if (uri->params)
    {
        free(uri->params);
    }

    free(uri);

    return 1;
}
