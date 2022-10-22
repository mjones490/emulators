all : dynosaur lib8080.so lib6502.so lib65c02.so

OBJECTS = dynosaur.o shell_commands.o video.o bus.o keyboard.o sound.o
OBJECTS := $(OBJECTS:%=obj/%)
LIBS = utils Stuff shell SDL2 SDL2_mixer readline dl jLib
LIBS := $(LIBS:%=-l%)

INCLUDE = ./include ../../Stuff ../../json ../include ../8080_cpu/include ../6052_cpu.include

CFLAGS = -Wall -g -fPIC -MMD -MP $(addprefix -I , $(INCLUDE))

.SECONDARY :
dynosaur : $(OBJECTS)
	$(MAKE) -C ../utilities
	$(CC) -Wall $(OBJECTS) -L../lib -L../../Stuff -L../../json $(LIBS) -o dynosaur

obj/%.o : src/%.c | obj/
	$(CC) $(CFLAGS) -c $< -o $@

obj/ :
	mkdir -v obj

lib8080.so : obj/8080_plugin.o
	$(MAKE) -C ../8080_cpu
	$(CC) -Wall -shared -Wl,-soname,lib8080.so -L../lib obj/8080_plugin.o -l8080 -o lib8080.so

lib6502.so : obj/6502_plugin.o ../lib/lib6502.a
	$(CC) -Wall -shared -Wl,-soname,lib6502.so -L../lib obj/6502_plugin.o -l6502_shell -l6502 -o lib6502.so

lib65c02.so : obj/6502_plugin.o ../lib/lib65c02.a
	$(CC) -Wall -shared -Wl,-soname,lib65c02.so -L../lib obj/6502_plugin.o -l6502_shell -l65c02 -o lib65c02.so

clean: 
	$(RM) -v *.so dynosaur
	$(RM) -vr obj

rebuild : clean 
	$(MAKE)

everything:
	$(MAKE) -C .. rebuild

-include obj/*.d
