INC_DIR = ../include
LIB_DIR = ../lib

CFLAGS = -Wall -g -MMD -D_REENTRANT -O -I$(INC_DIR)
LFLAGS = -Wall -L$(LIB_DIR)

all : build_libs shell

build_libs :
	cd ../shell && $(MAKE)

.SECONDARY :

clean :
	rm -vf shell *.o *.d 
	cd ../shell && $(MAKE) clean

shell : 8080_cpu.o shell.o control.o init.o ../lib/libshell.a
	$(CC) $(LFLAGS) $^ -lshell -lreadline -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
