#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <ctype.h>
#include <dirent.h>
#include "shell.h"
#include "command.h"
#include "types.h"

static void save_image(char *filename, WORD start, WORD end)
{
    int fd;
    int bytes_written;
    BYTE *buffer;
    int i;
    int length = end - start;

    buffer = malloc(length);
    for (i = 0; i < length; i++)
        *(buffer + i) = shell_peek_byte(start + i);

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        printf("Error creating file %s.\n", filename);
        goto done;
    }

    bytes_written = write(fd, buffer, length);
    printf("%d bytes written.\n", bytes_written);
    close(fd);

done:
    free(buffer);
}

int save(int argc, char **argv)
{
    unsigned int start, end;
    char *filename;
    int matched;

    if (3 > argc) {
        printf("save start-end filename\n");
        return 0;
    }

    matched = sscanf(argv[1], "%4x-%4x", &start, &end);
    if (matched != 2 || (end - start) == 0) {
        printf("Bad address range.\n");
        return 0;
    }

    filename = argv[2];

    printf("Saving %d bytes starting at address %04x to %s.\n", (end - start), start, filename);
    save_image(filename, start, end);

    return 0;
}

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
        shell_poke_byte(address + i, *(buffer + i));

    free(buffer);
    printf("%d bytes read.\n", bytes_read);
    close(fd);
    return;
}

static int load(int argc, char **argv)
{
    unsigned int address;
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

static int clear(int argc, char **argv)
{
    struct winsize sz;
    int i;
    ioctl(0, TIOCGWINSZ, &sz);

    for (i = 0; i < sz.ws_row; i++)
        printf("\n");

    return 0;
}

static int ls(int argc, char **argv)
{
    DIR *dp;
    struct dirent *ep;
    int fd;
    struct stat sb;
    char file_path[512];

    dp = opendir(argc == 1? "./" : argv[1]);
    if (dp == NULL) {
        printf("Error opening directory.\n");
        return 0;
    }

    while (ep = readdir(dp)) {
        if (ep->d_name[0] == '.')
            continue;
        sprintf(file_path,"%s/%s", argv[1], ep->d_name);
        fd = open(file_path, O_RDONLY);
        if (-1 == fd)
            continue;
        if (-1 != fstat(fd, &sb)) {
            printf("%*ld %s\n", 8, sb.st_size, file_path);
        }
        close(fd);
    }

    closedir(dp);

    return 0;
}

void shell_set_extended_commands(unsigned int commands)
{
    if (commands & SEC_LOAD)
        shell_add_command("load", "Load image from disk.", load, false);

    if (commands & SEC_SAVE)
        shell_add_command("save", "Save image to disk.", save, false);

//    if (commands & SEC_LS)
//        shell_add_command("ls", "List directory.", ls, false);

    if (commands & SEC_CLEAR)
        shell_add_command("clear", "Clear screen.", clear, false);
}

