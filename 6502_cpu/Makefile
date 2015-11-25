LIB := 6502
LIB_OBJS := 6502_cpu control init instructions modes

SHELL_LIB = 6502_shell
SHELL_LIB_OBJS = 6502_shell_commands

EXE := shell
EXE_OBJS := shell 6502_shell_commands
EXE_LIBS := 6502 shell readline

INC_DIR = ../include
LIB_DIR = ../lib

CFLAGS = -Wall -g -MMD -MP -D_REENTRANT -O -I$(INC_DIR)
LFLAGS = -Wall -L$(LIB_DIR)

CP = cp -v

EXE_OBJS := $(EXE_OBJS:%=%.o)
LIB := $(LIB:%=lib%.a)
LIB_OBJS := $(LIB_OBJS:%=%.o)
SHELL_LIB := $(SHELL_LIB:%=lib%.a)
SHELL_LIB_OBJS := $(SHELL_LIB_OBJS:%=%.o)
EXE_LIBS := $(EXE_LIBS:%=-l%)

all : build_libs $(LIB) $(SHELL_LIB) $(EXE)

build_libs :
	cd ../shell && $(MAKE)
   
.SECONDARY :
 
clean :
	rm -vf $(EXE_OBJS) $(LIB_OBJS) $(EXE) $(LIB) *.d
	rm -vf $(SHELL_LIB_OBJS) $(SHELL_LIB)
	cd ../shell && $(MAKE) clean

$(EXE) : $(EXE_OBJS) $(LIB) ../lib/libshell.a
	$(CC) $(LFLAGS) $(EXE_OBJS) $(EXE_LIBS) -o $(EXE)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.a : $(LIB_OBJS) $(SHELL_LIB_OBJS)
	$(AR) crv $(LIB) $(LIB_OBJS)
	$(AR) crv $(SHELL_LIB) $(SHELL_LIB_OBJS)
	$(CP) -v $(LIB) $(LIB_DIR)
	$(CP) -v $(SHELL_LIB) $(LIB_DIR)

