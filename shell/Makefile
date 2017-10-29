LIB := shell
LIB_OBJS := shell command basic_commands  

EXE := shell
EXE_OBJS := simple_shell
EXE_LIBS := shell readline

INC_DIR = ../include
LIB_DIR = ../lib

CFLAGS = -Wall -g -MMD -MP -D_REENTRANT -I$(INC_DIR)
LFLAGS = -Wall -L$(LIB_DIR)

CP = cp -v

EXE_OBJS := $(EXE_OBJS:%=%.o)
LIB := $(LIB:%=lib%.a)
LIB_OBJS := $(LIB_OBJS:%=%.o)
EXE_LIBS := $(EXE_LIBS:%=-l%)

all : $(LIB)

.SECONDARY :
 
%.a : $(LIB_OBJS) 
	$(AR) crv $@ $(LIB_OBJS)
	mkdir -pv $(LIB_DIR)
	$(CP) $(LIB) $(LIB_DIR)

clean :
	rm -f $(EXE_OBJS) $(LIB_OBJS) $(EXE) $(LIB) *.d

$(EXE) : $(EXE_OBJS) $(LIB)
	$(CC) $(LFLAGS) $(EXE_OBJS) $(EXE_LIBS) -o $(EXE)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
