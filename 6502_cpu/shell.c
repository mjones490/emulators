#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include "cpu_types.h"
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

int fifo_out_fd = -1;
int fifo_in_fd = -1;

BYTE my_accessor(WORD address, bool read_byte, BYTE value)
{
    char buf[2];
    
    if (address == 0xfff8) {
        buf[0] = value;
        buf[1] = 0;
        if (fifo_out_fd == -1)
            fifo_out_fd = open("tmp_fifo_out", O_WRONLY | O_NONBLOCK);
        write(fifo_out_fd, buf, 2); 
        return value;
    }

    if (read_byte && address == 0xfff9) {
        if (fifo_in_fd == -1)
            fifo_in_fd = open("tmp_fifo_in", O_RDONLY | O_NONBLOCK);
        read(fifo_in_fd, buf, 2);
        return buf[0];
    }

    if (address >= 0xfffa) 
        return vectors[address - 0xfffa];

    if (address == 0xff00)
        clear_signal(SIG_IRQ);

    if (address >= ram_size)
        return 0x00;
    if (read_byte) {
        value = ram[address];
    } else {
        ram[address] = value;
    }
    
    return value;
}

static void load_ram()
{
    int fd;
    struct stat sb;

    // Open RAM file and map it...
    fd = open("ram.bin", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (-1 == fd) {
        printf("Error opening RAM file.\n");
        exit(1);
    }

    if (-1 == fstat(fd, &sb)) {
        printf("Could not determine size of file.\n");
        exit(1);
    }

    if (ram_size > sb.st_size) {
        printf("Generating new RAM file.\n");
        ram = malloc(ram_size);
        memset(ram, 0, ram_size);
        if (-1 == write(fd, ram, ram_size)) {
            printf("Unable to generate new RAM files.\n");
            exit(1);
        }
        free(ram);
    }

    ram = mmap(NULL, ram_size, PROT_READ | PROT_WRITE, 
        MAP_SHARED, fd, 0);
    if (MAP_FAILED == ram) {
        printf("Error %d mapping RAM file.\n", errno);
        exit(1);
    }
    
    close(fd);

}

static void load_image(char* file_name, WORD address)
{
    int fd;
    struct stat sb;
    int bytes_read;

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

    bytes_read = read(fd, &ram[address], sb.st_size);
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

    if (1 != sscanf(argv[1], "%4x", &address)) {
        printf("Invalid load address.\n");
    }

    filename = argv[2];
    load_image(filename, address);

    return 0;
}

int main(int argc, char **argv)
{
    mkfifo("tmp_fifo_out", 0666);
    mkfifo("tmp_fifo_in", 0666);
        
    load_ram();
    cpu_init(my_accessor);

    shell_set_accessor(my_accessor);
    shell_set_loop_cb(cycle);
    shell_initialize("shell");
    shell_add_command("load", "Load a binary file the given address.", load, false);
    cpu_shell_load_commands();
    cpu_set_signal(SIG_HALT);
    shell_loop();
    shell_finalize();
    printf("\n");
    close(fifo_out_fd);
    close(fifo_in_fd);
    return 0;
}
