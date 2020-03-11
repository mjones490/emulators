INC_DIR = ../include
LIB_DIR = ../lib
STUFF_DIR = ../../Stuff

CFLAGS = -Wall -fPIC -g -MMD -D_REENTRANT -O -I$(INC_DIR) -I$(STUFF_DIR)
LFLAGS = -Wall -L$(LIB_DIR)

CP = cp -v
RM = rm -vf

all : libutils.a

.SECONDARY :

clean :
	$(RM) *.o *.d lib*.a

rebuild : clean
	$(MAKE)

libutils.a : logging.o config.o
	$(AR) crv libutils.a logging.o config.o
	mkdir -pv $(LIB_DIR)
	$(CP) -v libutils.a $(LIB_DIR)

logging.o : logging.c
	$(CC) $(CFLAGS) -c logging.c -o logging.o

config.o : config.c
	$(CC) $(CFLAGS) -c config.c  -o config.o

