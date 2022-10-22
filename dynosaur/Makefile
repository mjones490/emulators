all : dynosaur lib8080.so lib6502.so lib65c02.so

OBJECTS = dynosaur.o shell_commands.o video.o bus.o keyboard.o sound.o
LIBS = utils Stuff shell SDL2 SDL2_mixer readline dl jLib
LIBS := $(LIBS:%=-l%)

INCLUDE = -I../../Stuff -I../../json -I../include

.SECONDARY :
dynosaur : $(OBJECTS)
	$(MAKE) -C ../utilities
	$(CC) -Wall $(OBJECTS) -L../lib -L../../Stuff -L../../json $(LIBS) -o dynosaur

dynosaur.o : dynosaur.c
	$(CC) -Wall -g  $(INCLUDE) -c dynosaur.c -o dynosaur.o

shell_commands.o : shell_commands.c
	$(CC) -Wall -g  $(INCLUDE) -c shell_commands.c -o shell_commands.o

video.o : video.c
	$(CC) -Wall -g  $(INCLUDE) -c video.c -o video.o

bus.o : bus.c
	$(CC) -Wall -g  $(INCLUDE) -c bus.c -o bus.o

keyboard.o : keyboard.c
	$(CC) -Wall -g  $(INCLUDE) -c keyboard.c -o keyboard.o

sound.o : sound.c
	$(CC) -Wall -g $(INCLUDE)  -c sound.c -o sound.o

lib8080.so : 8080_plugin.o
	$(MAKE) -C ../8080_cpu
	$(CC) -Wall -shared -Wl,-soname,lib8080.so -L../lib 8080_plugin.o -l8080 -o lib8080.so

8080_plugin.o : 8080_plugin.c
	$(CC) -Wall -fPIC  $(INCLUDE) -I../8080_cpu/include -c 8080_plugin.c -o 8080_plugin.o

lib6502.so : 6502_plugin.o ../lib/lib6502.a
	$(CC) -Wall -shared -Wl,-soname,lib6502.so -L../lib 6502_plugin.o -l6502_shell -l6502 -o lib6502.so

6502_plugin.o : 6502_plugin.c
	$(CC) -Wall -fPIC  $(INCLUDE) -I../6502_cpu/include -c 6502_plugin.c -o 6502_plugin.o

lib65c02.so : 6502_plugin.o ../lib/lib65c02.a
	$(CC) -Wall -shared -Wl,-soname,lib65c02.so -L../lib 6502_plugin.o -l6502_shell -l65c02 -o lib65c02.so

clean: 
	rm -vf *.o *.so dynosaur

rebuild : clean 
	$(MAKE)

everything:
	$(MAKE) -C .. rebuild
