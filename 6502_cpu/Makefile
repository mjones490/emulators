LIB_6502 := 6502
LIB_65C02 := 65c02
LIB_6502_OBJS := 6502_cpu control init instructions 6502_instructions modes 
LIB_65C02_OBJS := 6502_cpu control init instructions 65c02_instructions modes 

SHELL_6502_LIB = 6502_shell
SHELL_6502_LIB_OBJS = 6502_shell_commands
SHELL_65C02_LIB = 65c02_shell

SHELL_6502_EXE := shell_6502
SHELL_65C02_EXE := shell_65c02
SHELL_EXE_OBJS := shell 6502_shell_commands
SHELL_EXE_LIBS :=  readline

INC_DIR = ./include ../include 
LIB_DIR = ../lib

CFLAGS = -Wall -g -MMD -MP -D_REENTRANT -fPIC -O $(addprefix -I ,$(INC_DIR))
LFLAGS = -Wall -L$(LIB_DIR)

CP = cp -v

SHELL_EXE_OBJS := $(SHELL_EXE_OBJS:%=obj/%.o)
LIB_6502 := $(LIB_6502:%=lib%.a)
LIB_65C02 := $(LIB_65C02:%=lib%.a)
LIB_6502_OBJS := $(LIB_6502_OBJS:%=obj/%.o)
LIB_65C02_OBJS := $(LIB_65C02_OBJS:%=obj/%.o)
SHELL_6502_LIB := $(SHELL_6502_LIB:%=lib%.a)
SHELL_65C02_LIB := $(SHELL_65C02_LIB:%=lib%.a)
SHELL_6502_LIB_OBJS := $(SHELL_6502_LIB_OBJS:%=obj/%.o)
SHELL_EXE_LIBS := $(SHELL_EXE_LIBS:%=-l%)

all : build_libs $(LIB_6502) $(LIB_65C02) $(SHELL_6502_LIB) $(SHELL_65C02_LIB) $(SHELL_6502_EXE) $(SHELL_65C02_EXE)

build_libs :
	cd ../shell && $(MAKE)
   
.SECONDARY :
 
clean :
	$(RM) -v $(SHELL_6502_LIB) $(SHELL_65C02_LIB) $(LIB_65C02) $(LIB_6502)
	$(RM) -v shell_6502 shell_65c02
	$(RM) -vr obj
	cd ../shell && $(MAKE) clean

rebuild : clean
	$(MAKE)

$(SHELL_6502_EXE) : $(SHELL_EXE_OBJS) $(SHELL_EXE_LIBS) ../lib/libshell.a lib6502.a 
	$(CC) $(LFLAGS) $(SHELL_EXE_OBJS) -l6502 -lshell $(SHELL_EXE_LIBS) -o $(SHELL_6502_EXE)

$(SHELL_65C02_EXE) : $(SHELL_EXE_OBJS) $(SHELL_EXE_LIBS) ../lib/libshell.a lib65c02.a 
	$(CC) $(LFLAGS) $(SHELL_EXE_OBJS) -l65c02 -lshell $(SHELL_EXE_LIBS) -o $(SHELL_65C02_EXE)

obj/%.o :  src/%.c | obj/
	$(CC) $(CFLAGS) -c $< -o $@

obj/ :
	@echo Making obj dir...
	mkdir obj

$(LIB_6502) : $(LIB_6502_OBJS)
	$(AR) crv $(LIB_6502) $(LIB_6502_OBJS)
	mkdir -pv $(LIB_DIR)
	$(CP) -v $(LIB_6502) $(LIB_DIR)

$(LIB_65C02) : $(LIB_65C02_OBJS)
	$(AR) crv $(LIB_65C02) $(LIB_65C02_OBJS)
	mkdir -pv $(LIB_DIR)
	$(CP) -v $(LIB_65C02) $(LIB_DIR)


$(SHELL_6502_LIB) : $(SHELL_6502_LIB_OBJS)
	$(AR) crv $(SHELL_6502_LIB) $(SHELL_6502_LIB_OBJS)
	$(CP) -v $(SHELL_6502_LIB) $(LIB_DIR)

$(SHELL_65C02_LIB) : $(SHELL_6502_LIB_OBJS)
	$(AR) crv $(SHELL_65C02_LIB) $(SHELL_6502_LIB_OBJS)
	$(CP) -v $(SHELL_65C02_LIB) $(LIB_DIR)

-include obj/*.d
