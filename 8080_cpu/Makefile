INC_DIR = ../include
LIB_DIR = ../lib

CFLAGS = -Wall -fPIC -g -MMD -D_REENTRANT -O -I$(INC_DIR)
LFLAGS = -Wall -L$(LIB_DIR)

CP = cp -v

all : build_libs shell

.SECONDARY :

build_libs :
	$(MAKE) -C ../shell

clean :
	rm -vf shell *.o *.d *.a *.so 
	$(MAKE) -C ../shell clean

rebuild : clean
	$(MAKE)

shell : shell.o ../lib/libshell.a lib8080.a
	$(CC) $(LFLAGS) $^ -lshell -lreadline -o $@

lib8080.a : 8080_cpu.o 8080_shell_commands.o control.o init.o instructions.o
	$(AR) crv lib8080.a 8080_cpu.o 8080_shell_commands.o control.o init.o instructions.o
	mkdir -pv ../lib
	$(CP) lib8080.a ../lib

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
