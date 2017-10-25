# Add additional project sources like this:
# SRCS += X.c
#
# all the files will be generated with this name (main.o, etc)

#Name of Project

CC = gcc
CFLAGS = -Wall -lpthread -levent -std=gnu99
XFLAGS = -lX11

.PHONY: all

all: main s4382911_usershell s4382911_cag s4382911_cagdisplay 

main: main.o
	$(CC) $(CFLAGS) main.o -o main -g
    
s4382911_usershell: s4382911_usershell.o
	$(CC) $(CFLAGS) s4382911_usershell.o -o usershell -g

s4382911_cag: s4382911_cag.o
	$(CC) $(CFLAGS) s4382911_cag.o -o cag -g
    
s4382911_cagdisplay: s4382911_cagdisplay.o
	$(CC) $(CFLAGS) $(XFLAGS) s4382911_cagdisplay.o -o cagdisplay -g

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o -g
    
s4382911_usershell.o: s4382911_usershell.c
	$(CC) $(CFLAGS) -c s4382911_usershell.c -o s4382911_usershell.o -g
    
s4382911_cag.o: s4382911_cag.c
	$(CC) $(CFLAGS) -c s4382911_cag.c -o s4382911_cag.o -g
    
s4382911_cagdisplay.o: s4382911_cagdisplay.c
	$(CC) $(CFLAGS) $(XFLAGS) -c s4382911_cagdisplay.c -o s4382911_cagdisplay.o -g
