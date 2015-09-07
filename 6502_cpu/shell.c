#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include "6502_cpu.h"
#include "6502_shell.h"

BYTE my_accessor(WORD address, bool read, BYTE value);
//static inline BYTE get_byte(WORD address)
//{
//   return my_accessor(address, true, 0);
//}


static void cycle()
{
   // static int clocks = 0;
    usleep(1);
    if (!cpu_get_signal(SIG_HALT))
        cpu_execute_instruction();
}

// Testing section
const int ram_size = 8192;

BYTE *ram;
BYTE vectors[] = {0x40, 0x10, 0x00, 0x12, 0x00, 0x10};

BYTE my_accessor(WORD address, bool read, BYTE value)
{
    if (address == 0xfff8) {
        printf("%c", value);
        fflush(stdout);
        return value;
    } 
    
    if (address >= 0xfffa) 
        return vectors[address - 0xfffa];

    if (address >= ram_size)
        return 0x00;
    if (read) {
        value = ram[address];
    } else {
        ram[address] = value;
    }
    
    return value;
}

static void load_ram()
{
    int fd;

    // Open RAM file and map it...
    fd = open("ram.bin", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (-1 == fd) {
        printf("Error opening RAM file.\n");
        exit(1);
    }

    ram = mmap(NULL, ram_size, PROT_READ | PROT_WRITE, 
        MAP_SHARED, fd, 0);
    if (MAP_FAILED == ram) {
        printf("Error %d mapping RAM file.\n", errno);
        exit(1);
    }

}

int main(int argc, char **argv)
{
    load_ram();
    cpu_init(my_accessor);

    shell_set_accessor(my_accessor);
    shell_set_loop_cb(cycle);
    shell_initialize("shell");
    cpu_shell_load_commands();
    cpu_set_signal(SIG_HALT);
    shell_loop();
    shell_finalize();
    printf("\n");
    return 0;
}
