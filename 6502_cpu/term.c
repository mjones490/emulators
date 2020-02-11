#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <termios.h>

struct termios orig_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int getch()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}
int main(int argc, char **argv)
{
    int fd_in = -1;
    int fd_out = -1;
    char buf[2];

    buf[1] = 0;

    mkfifo("tmp_fifo_out", 0666);
    mkfifo("tmp_fifo_in", 0666);

    fd_in = open("tmp_fifo_out", O_RDONLY | O_NONBLOCK);
    if (fd_in == -1)
        printf("Unable to open fd_in.\n");

    set_conio_terminal_mode();
    
    while (1) {
        if (read(fd_in, buf, 2) > 0) {
            printf(buf);
            fflush(stdout);
            buf[0] = 0;
        }

        if (kbhit()) {
            buf[0] = getch();
            if (fd_out == -1) {
                fd_out = open("tmp_fifo_in", O_WRONLY | O_NONBLOCK);
            }
            write(fd_out, buf, 2);
            
            buf[0] = 0;
        }
    }

    return 0;
}
