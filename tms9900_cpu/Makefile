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
	rm -vf shell *.o *.d *.a 
	$(MAKE) -C ../shell clean

rebuild : clean
	$(MAKE)

shell : shell.o format.o instruction.o ../lib/libshell.a
	$(CC) $(LFLAGS) $^ -lshell -lreadline -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
