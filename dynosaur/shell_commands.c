#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "types.h"
#include "dynosaur.h"
#include "bus.h"

static void load_image(char* file_name, WORD address)
{
    int fd;
    struct stat sb;
    int bytes_read;
    BYTE* buffer;
    int i;

    // Open file
    fd = open(file_name, O_RDONLY);
    if (-1 == fd) {
        printf("Error opening file %s.\n", file_name);
        return;
    }

    if (-1 == fstat(fd, &sb)) {
        printf("Could not determine size of file.\n");
        close(fd);
        return;
    }    

    buffer = malloc(sb.st_size);
    printf("Reading %ld bytes...\n", sb.st_size);
    bytes_read = read(fd, buffer, sb.st_size);

    for (i = 0; i < bytes_read; i++)
        put_byte(address + i, *(buffer + i));

    free(buffer);
    printf("%d bytes read.\n", bytes_read);
    close(fd);
    return;
}

static int load(int argc, char **argv)
{
    WORD address;
    char* filename;

    if (3 > argc) {
        printf("load addr filename\n");
        return 0;
    }

    if (1 != sscanf(argv[1], "%4hx", &address)) {
        printf("Invalid load address.\n");
    }

    filename = argv[2];
    load_image(filename, address);

    return 0;
}

static int step(int argc, char **argv)
{
    int matched;
    WORD new_pc;
    if (1 < argc) {
        matched = sscanf(argv[1], "%4x", &new_pc);
        if (matched)
            cpu->set_PC(new_pc);
    }

    cpu->execute_instruction();
    cpu->show_registers();
    cpu->disassemble(cpu->get_PC());
    return 0;
}

static int next(int argc, char **argv)
{
    BYTE code = get_byte(cpu->get_PC());
    nextpoint = cpu->get_PC() + cpu->get_instruction_size(code);
    cpu->set_halted(false);
    return 0;
}

static int go(int argc, char **argv)
{
    int matched;
    WORD new_pc;
    if (1 < argc) {
        matched = sscanf(argv[1], "%4x", &new_pc);
        if (matched)
            cpu->set_PC(new_pc);
    }

    cpu->set_halted(false);
    return 0;
}

static int halt(int argc, char **argv)
{
    cpu->set_halted(true);
    return 0;
}

static int set_breakpoint(int argc, char **argv)
{
    WORD temp;

    if (argc == 2) {
        if (1 == sscanf(argv[1], "%04hx", &temp))
            breakpoint = temp;
    }

    printf("Breakpoint set at %04x.\n", breakpoint);

    return 0;
}

static int disassemble(int argc, char **argv)
{
    int matched;
    int start;
    int end;
    static WORD address;
    static WORD extend = 1;

    if (1 < argc) { 
        matched = sscanf(argv[1], "%4x-%4x", &start, &end);
        if (matched) {
            address = start;
            if (matched == 1) { 
                end = address;
                extend = 1;
            } else {
                extend = end - address;
            }
        }
    } else {
        end = address + extend;
    }

    do {
        address = cpu->disassemble(address);
    } while (address < end);

    return 0;
}

static int registers(int argc, char **argv)
{
    int tmp;
    char *reg_name;
    int arg_no = 1;

    while (argc > arg_no) {
        reg_name = argv[arg_no++];
        if (1 == sscanf(argv[arg_no++], "%4x", &tmp)) 
            cpu->set_register(reg_name, (WORD) tmp);
    }
    
    cpu->show_registers();
    return 0;
}

static int out_port(int argc, char **argv)
{
    int tmp;
    BYTE port;
    BYTE value;
    int parms = 0;

    if (argc == 3) {
        if (1 == sscanf(argv[1], "%2x", &tmp)) {
            parms++;
            port = tmp;
        }
        
        if (1 == sscanf(argv[2], "%2x", &tmp)) {
            parms++;
            value = tmp;
        }

        if (parms == 2)
            port_accessor(port, false, value);
    }

    return 0;
}

void shell_load_dynosaur_commands()
{
    shell_add_command("step", "Execute single instruction.", step, true);
    shell_add_command("go", "Start CPU.", go, false);
    shell_add_command("halt", "Halt CPU", halt, false);
    shell_add_command("registers", "View/Set CPU registers", registers, false);
    shell_add_command("disassemble", "Disassemble instructions", disassemble, true);
    shell_add_command("breakpoint", "View/Set breakpoint", set_breakpoint, false);
    shell_add_command("load", "Load a binary image file into RAM", load, false);
    shell_add_command("out", "Write value to port", out_port, false);
    shell_add_command("next", "Step, preceding through subroutine calls.", next, true);
}

