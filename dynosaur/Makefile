all : dynosaur lib8080.so lib6502.so

.SECONDARY :

dynosaur : dynosaur.o
	$(CC) -Wall dynosaur.o -ldl -o dynosaur

dynosaur.o : dynosaur.c
	$(CC) -Wall -I../include -c dynosaur.c -o dynosaur.o

lib8080.so : 8080_plugin.o
	$(CC) -Wall -shared -Wl,-soname,lib8080.so -L../lib 8080_plugin.o -l8080 -lshell -lreadline -o lib8080.so

8080_plugin.o : 8080_plugin.c
	$(CC) -Wall -fPIC -I../include -I../8080_cpu -c 8080_plugin.c -o 8080_plugin.o

lib6502.so : 6502_plugin.o
	$(CC) -Wall -shared -Wl,-soname,lib6502.so -L../lib 6502_plugin.o -l6502_shell -l6502 -lshell -lreadline -o lib6502.so

6502_plugin.o : 6502_plugin.c
	$(CC) -Wall -fPIC -I../include -I../6502_cpu -c 6502_plugin.c -o 6502_plugin.o

clean: 
	rm -v *.o *.so dynosaur

