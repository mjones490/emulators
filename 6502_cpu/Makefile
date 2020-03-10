LIB := 6502
LIB_OBJS := 6502_cpu control init instructions modes

SHELL_LIB = 6502_shell
SHELL_LIB_OBJS = 6502_shell_commands

SHELL_EXE := shell
SHELL_EXE_OBJS := shell 6502_shell_commands
SHELL_EXE_LIBS := 6502 shell readline

TERM_EXE := term

INC_DIR = ../include
LIB_DIR = ../lib

CFLAGS = -Wall -g -MMD -MP -D_REENTRANT -O -I$(INC_DIR)
LFLAGS = -Wall -L$(LIB_DIR)

CP = cp -v

SHELL_EXE_OBJS := $(SHELL_EXE_OBJS:%=%.o)
LIB := $(LIB:%=lib%.a)
LIB_OBJS := $(LIB_OBJS:%=%.o)
SHELL_LIB := $(SHELL_LIB:%=lib%.a)
SHELL_LIB_OBJS := $(SHELL_LIB_OBJS:%=%.o)
SHELL_EXE_LIBS := $(SHELL_EXE_LIBS:%=-l%)

all : build_libs $(LIB) $(SHELL_LIB) $(SHELL_EXE) $(TERM_EXE)

build_libs :
	cd ../shell && $(MAKE)
   
.SECONDARY :
 
clean :
	rm -vf $(SHELL_EXE_OBJS) $(LIB_OBJS) $(SHELL_EXE) $(LIB) $(TERM_EXE) *.d
	rm -vf $(SHELL_LIB_OBJS) $(SHELL_LIB)
	cd ../shell && $(MAKE) clean

$(SHELL_EXE) : $(SHELL_EXE_OBJS) $(LIB) ../lib/libshell.a
	$(CC) $(LFLAGS) $(SHELL_EXE_OBJS) $(SHELL_EXE_LIBS) -o $(SHELL_EXE)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB) : $(LIB_OBJS)
	$(AR) crv $(LIB) $(LIB_OBJS)
	mkdir -pv $(LIB_DIR)
	$(CP) -v $(LIB) $(LIB_DIR)


$(SHELL_LIB) : $(SHELL_LIB_OBJS)
	$(AR) crv $(SHELL_LIB) $(SHELL_LIB_OBJS)
	$(CP) -v $(SHELL_LIB) $(LIB_DIR)

