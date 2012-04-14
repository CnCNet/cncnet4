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

#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include "list.h"
#include "config.h"

#define STR_TRUE(s) (s != NULL && (s[0] == '1' || s[0] == 't' || s[0] == 'T' || s[0] == 'y' || s[0] == 'Y' || s[0] == 'e' || s[0] == 'E'))

#define CONFIG_SECTION "CnCNet4"
#define CONFIG_PATH ".\\cncnet.ini"

typedef struct Config
{
    char key[32];
    char value[128];
    struct Config *next;
} Config;

static Config *config = NULL;

void config_init()
{
    config_set_default("Host", "server.cncnet.org");
    config_set_default("Port", "9001");
    config_set_default("P2P", "");
    config_set_default("Executable", "");
    config_set_default("Dll", "");
    config_set_default("Timeout", "3");
    config_set_default("ExtractDll", "true");
    config_set_default("AutoUpdate", "true");
}

void config_save()
{
    Config *i;

    LIST_FOREACH(config, i)
    {
        WritePrivateProfileString(CONFIG_SECTION, i->key, i->value, CONFIG_PATH);
    }
}

void config_free()
{
    LIST_FREE(config);
}

Config *config_list_find(const char *key)
{
    Config *i;
    Config *res = NULL;

    LIST_FOREACH(config, i)
    {
        if (strcmp(i->key, key) == 0)
        {
            res = i;
            break;
        }
    }

    if (res == NULL)
    {
        res = LIST_NEW(Config);
        strncpy(res->key, key, sizeof res->key);
        LIST_INSERT(config, res);
    }

    return res;
}

const char *config_get(const char *key)
{
    Config *cfg = config_list_find(key);

    GetPrivateProfileString(CONFIG_SECTION, cfg->key, cfg->value, cfg->value, sizeof cfg->value, CONFIG_PATH);

    return cfg->value;
}

int config_get_int(const char *key)
{
    return atoi(config_get(key));
}

int config_get_bool(const char *key)
{
    return STR_TRUE(config_get(key));
}

void config_set(const char *key, const char *value)
{
    Config *cfg = config_list_find(key);
    strncpy(cfg->value, value, sizeof cfg->value);
}

void config_set_default(const char *key, const char *value)
{
    if (strlen(config_get(key)) == 0)
    {
        config_set(key, value);
    }
}
