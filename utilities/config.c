#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <Stuff.h>
#include "config.h"
#include "logging.h"

struct config_struct *config;

char *get_config_string(char *section_name, char *key_name)
{
    struct section_struct *section;
    struct key_struct *key;
    char *value = NULL;

    section = find_section(config->section, section_name);
    if (section != NULL) {
        config->section = section;
        key = find_key(section->key, key_name);
        if (key != NULL) {
            section->key = key;
            value = key->value;
        }
    }

    return value;
}

bool get_config_bool(char *section_name, char *key_name)
{
    bool value = false;
    char *string;

    string = get_config_string(section_name, key_name);
    if (string != NULL) {
        if (0 == strncasecmp("true", string, 4))
            value = true;
    }

    return value;
}

int string_to_int(char *string)
{
    int value = 0;

    if (string != NULL)
        sscanf(string, "%d", &value);
        
    return value; 
}

int get_config_int(char *section_name, char *key_name)
{
    char *string;
    string = get_config_string(section_name, key_name);
    return string_to_int(string);
}

Uint32 string_to_hex(char *string)
{
    Uint32 value = 0;
    if (string != NULL)
        sscanf(string, "%x", &value);

    return value;
}

Uint32 get_config_hex(char *section_name, char *key_name)
{
    char *string;
    string = get_config_string(section_name, key_name);
    return string_to_hex(string);
}

char *split_string(char *string, char *buffer, char splitter)
{
    while (*string && *string != splitter) 
        *(buffer++) = *(string++);
   
    *(buffer++) = 0;
     
    if (*string == splitter)
        string++;

    return string;
}

void init_config(char *cfg_filename)
{
    char *message = NULL;   
    config = config_open(cfg_filename);
    if (config == NULL) {
        LOG_FTL("Error opening %s.\n", cfg_filename);
    }
}

void finalize_config()
{
    LOG_INF("Closing config.\n");
    config_close(config);
}

