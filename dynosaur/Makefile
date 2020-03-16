all : dynosaur lib8080.so lib6502.so

.SECONDARY :
dynosaur : dynosaur.o shell_commands.o video.o
	$(MAKE) -C ../utilities
	$(CC) -Wall dynosaur.o shell_commands.o video.o -L../lib -L../../Stuff -lutils -lStuff -lshell -lSDL2 -lreadline -ldl -o dynosaur

dynosaur.o : dynosaur.c
	$(CC) -Wall -g -I../include -c dynosaur.c -o dynosaur.o

shell_commands.o : shell_commands.c
	$(CC) -Wall -g -I../include -c shell_commands.c -o shell_commands.o

video.o : video.c
	$(CC) -Wall -g -I../include -I../../Stuff -c video.c -o video.o

lib8080.so : 8080_plugin.o
	$(MAKE) -C ../8080_cpu
	$(CC) -Wall -shared -Wl,-soname,lib8080.so -L../lib 8080_plugin.o -l8080 -o lib8080.so

8080_plugin.o : 8080_plugin.c
	$(CC) -Wall -fPIC -I../include -I../8080_cpu -c 8080_plugin.c -o 8080_plugin.o

lib6502.so : 6502_plugin.o ../lib/lib6502.a
	$(CC) -Wall -shared -Wl,-soname,lib6502.so -L../lib 6502_plugin.o -l6502_shell -l6502 -o lib6502.so

6502_plugin.o : 6502_plugin.c
	$(CC) -Wall -fPIC -I../include -I../6502_cpu -c 6502_plugin.c -o 6502_plugin.o

clean: 
	rm -vf *.o *.so dynosaur

rebuild : clean 
	$(MAKE)

