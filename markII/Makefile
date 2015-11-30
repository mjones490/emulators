EXE := markII
EXE_OBJS := init bus config logging video
EXE_LIBS := 6502 6502_shell Stuff shell readline SDL2

INC_DIR = ../include Stuff /usr/include/SDL2
LIB_DIR = ../lib . 

CFLAGS = -Wall -g -MMD -MP -D_REENTRANT  $(INC_DIR:%=-I%)
LFLAGS = -Wall $(LIB_DIR:%=-L%)

CP = cp -v

EXE_OBJS := $(EXE_OBJS:%=%.o)
EXE_LIBS := $(EXE_LIBS:%=-l%)

all : build_libs $(EXE)

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

