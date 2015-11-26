#ifndef __CONFIG_H
#define __CONFIG_H
//#include <SDL.h> // For Uint32

typedef unsigned int Uint32;
void init_config();

int get_config_int(char *section_name, char *key_name);
Uint32 get_config_hex(char *section_name, char *key_name);
char *get_config_string(char *section_name, char *key_name);
bool get_config_bool(char *section_name, char *key_name);
Uint32 string_to_hex(char *string);
int string_to_int(char *string);
char *split_string(char *string, char *buffer, char splitter);

#endif
