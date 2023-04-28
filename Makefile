CC=gcc
#
# If you want to enable the "standard" server behaviour of forking a process
# to handle each incoming socket connection, then define the symbol DOFORK
# using the following line. 
# CFLAGS=-g -Wall -std=gnu11 -DDOFORK
CFLAGS=-g -Wall -std=gnu11

all: mysmtpd 

test:   mysmtpd
	./test.sh

mysmtpd: mysmtpd.o netbuffer.o mailuser.o server.o util.o
	gcc $(CFLAGS) mysmtpd.o netbuffer.o mailuser.o server.o util.o   -o mysmtpd

mysmtpd.o: mysmtpd.c netbuffer.h mailuser.h server.h
netbuffer.o: netbuffer.c netbuffer.h
mailuser.o: mailuser.c mailuser.h
server.o: server.c server.h
util.o: util.h

clean:
	-rm -rf mysmtpd mysmtpd.o netbuffer.o mailuser.o server.o util.o
tidy: clean
	-rm -rf *~ out.s.? mail.store
