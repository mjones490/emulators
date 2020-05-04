#ifndef __SHELL_EXTENDED_COMMANDS_H
#define __SHELL_EXTENDED_COMMANDS_H

#define SEC_LOAD            0x00000001
#define SEC_SAVE            0x00000002
#define SEC_CD              0x00000004
#define SEC_LS              0x00000008
#define SEC_CLEAR           0x00000010
#define SEC_ALL_COMMANDS    0xffffffff


void shell_set_extended_commands(unsigned int commands);

#endif
