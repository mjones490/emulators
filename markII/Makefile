EXE := markII
EXE_OBJS := bus \
            cpu_iface \
            disk \
            disk_iface \
            dos33 \
            init \
            keyboard \
            markII_commands \
            mmu \
            slot \
            sound \
            video

EXE_LIBS := 6502 \
            6502_shell \
            utils \
            Stuff \
            shell \
            readline \
            SDL2 \
            SDL2_mixer \
            m

INC_DIR = ./include \
            ../include \
          ../../Stuff \
          /usr/include/SDL2

LIB_DIR = ../../Stuff ../lib . 

CFLAGS = -Wall -g -O0 -MMD -MP -D_REENTRANT  $(INC_DIR:%=-I%)
LFLAGS = -Wall $(LIB_DIR:%=-L%)

CP = cp -v

EXE_OBJS := $(EXE_OBJS:%=obj/%.o)
EXE_LIBS := $(EXE_LIBS:%=-l%)


all : build_libs $(EXE)

build_libs :
	$(MAKE) -C ../shell
	$(MAKE) -C ../6502_cpu
	$(MAKE) -C ../utilities

.SECONDARY :
 
clean :
	$(RM) -v $(EXE)
	$(RM) -vr obj
	$(MAKE) -C ../shell clean
	$(MAKE) -C ../6502_cpu clean

rebuild : clean
	$(MAKE)

$(EXE) : $(EXE_OBJS)
	$(CC) $(LFLAGS) $(EXE_OBJS) $(EXE_LIBS) -o $(EXE)

obj/%.o : src/%.c | obj/
	$(CC) $(CFLAGS) -c $< -o $@

obj/ :
	mkdir -v obj

docs :
	doxygen Doxyfile

-include obj/*.d
