CC=gcc
CFLAGS=-I../include
LDFLAGS=

all: disp dispd menud disp-bwm

disp: disp.c
	$(CC) $(CFLAGS) $? $(LDFLAGS) -o $@

dispd: dispd.c
	$(CC) $(CFLAGS) $? $(LDFLAGS) -o $@

menud: menud.c
	$(CC) $(CFLAGS) $? $(LDFLAGS) -o $@

disp-bwm: disp-bwm.c
	$(CC) $(CFLAGS) $? $(LDFLAGS) -o $@


clean:
	rm -rf core *.o disp dispd menud disp-bwm

install:
