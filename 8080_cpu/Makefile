INC_DIR = ./include ../include
LIB_DIR = ../lib

CFLAGS = -Wall -g -MMD -D_REENTRANT -fPIC -O $(addprefix -I ,$(INC_DIR))
LFLAGS = -Wall -L$(LIB_DIR)

CP = cp -v

LIB_8080_OBJS := 8080_cpu 8080_shell_commands control init instructions
LIB_8080_OBJS := $(LIB_8080_OBJS:%=obj/%.o) 
all : build_libs shell

.SECONDARY :

build_libs :
	$(MAKE) -C ../shell

clean :
	$(RM) -v shell *.a *.so 
	$(RM) -rv obj
	$(MAKE) -C ../shell clean

rebuild : clean
	$(MAKE)

shell : obj/shell.o ../lib/libshell.a lib8080.a
	$(CC) $(LFLAGS) $^ -lshell -lreadline -o $@

lib8080.a : $(LIB_8080_OBJS)
	$(AR) crv lib8080.a $(LIB_8080_OBJS)
	mkdir -pv ../lib
	$(CP) lib8080.a ../lib

obj/%.o : src/%.c | obj/
	$(CC) $(CFLAGS) -c $< -o $@

obj/ :
	mkdir -v obj

-include obj/*.d
