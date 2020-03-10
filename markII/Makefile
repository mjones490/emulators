EXE := markII
EXE_OBJS := bus \
            config \
            cpu_iface \
            disk \
            disk_iface \
            dos33 \
            init \
            keyboard \
            logging \
            markII_commands \
            mmu \
            slot \
            sound \
            video

EXE_LIBS := 6502 \
            6502_shell \
            Stuff \
            shell \
            readline \
            SDL2 \
            SDL2_mixer \
            m

INC_DIR = ../include \
          ../../Stuff \
          /usr/include/SDL2

LIB_DIR = ../../Stuff ../lib . 

CFLAGS = -Wall -g -O0 -MMD -MP -D_REENTRANT  $(INC_DIR:%=-I%)
LFLAGS = -Wall $(LIB_DIR:%=-L%)

CP = cp -v

EXE_OBJS := $(EXE_OBJS:%=%.o)
EXE_LIBS := $(EXE_LIBS:%=-l%)


all : build_libs $(EXE)

build_libs :
	$(MAKE) -C ../shell
	$(MAKE) -C ../6502_cpu
   
.SECONDARY :
 
clean :
	rm -vf $(EXE_OBJS) $(LIB_OBJS) $(EXE) $(LIB) *.d
	rm -vf $(SHELL_LIB_OBJS) $(SHELL_LIB)
	$(MAKE) -C ../shell clean
	$(MAKE) -C ../6502_cpu clean

$(EXE) : $(EXE_OBJS)
	$(CC) $(LFLAGS) $(EXE_OBJS) $(EXE_LIBS) -o $(EXE)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

docs :
	doxygen Doxyfile
