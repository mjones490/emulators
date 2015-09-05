LIB := 6502
LIB_OBJS := 6502_cpu control init instructions modes

EXE := shell
EXE_OBJS := shell
EXE_LIBS := 6502 shell readline

INC_DIR = ../include
LIB_DIR = ../lib

CFLAGS = -Wall -g -MMD -MP -D_REENTRANT -O -I$(INC_DIR)
LFLAGS = -Wall -L$(LIB_DIR)

CP = cp -v

EXE_OBJS := $(EXE_OBJS:%=%.o)
LIB := $(LIB:%=lib%.a)
LIB_OBJS := $(LIB_OBJS:%=%.o)
EXE_LIBS := $(EXE_LIBS:%=-l%)

all : build_libs $(LIB)

build_libs :
	cd ../shell && $(MAKE)
   
.SECONDARY :
 
clean :
	rm -f $(EXE_OBJS) $(LIB_OBJS) $(EXE) $(LIB) *.d
	cd ../shell && $(MAKE) clean

$(EXE) : $(EXE_OBJS) $(LIB) ../lib/libshell.a
	$(CC) $(LFLAGS) $(EXE_OBJS) $(EXE_LIBS) -o $(EXE)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.a : $(LIB_OBJS) 
	$(AR) crv $@ $(LIB_OBJS)
	$(CP) $(LIB) $(LIB_DIR)

