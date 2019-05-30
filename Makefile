CC=gcc
CCLD=gcc
CFLAGS=-Wall -g
LDFLAGS=-Wall -g

all: lspci-ng

clean:
	rm -f *.o lspci-ng

lspci-ng: \
	dmi.o lspci.o main.o nvidia-smi.o portdb.o prun.o
	$(CCLD) $(LDFLAGS) $^ -o $@

dmi.o: \
	dmi.c \
	dmi.h \
	main.h \
	prun.h
	$(CC) $(CFLAGS) $< -o $@ -c

lspci.o: \
	lspci.c \
	dmi.h \
	lspci.h \
	main.h \
	nvidia-smi.h \
	portdb.h \
	prun.h
	$(CC) $(CFLAGS) $< -o $@ -c

main.o: \
	main.c \
	dmi.h \
	lspci.h \
	nvidia-smi.h
	$(CC) $(CFLAGS) $< -o $@ -c

nvidia-smi.o: \
	nvidia-smi.c \
	main.h \
	nvidia-smi.h
	$(CC) $(CFLAGS) $< -o $@ -c

portdb.o: \
	portdb.c \
	portdb-static.c \
	lspci.h \
	main.h \
	portdb.h
	$(CC) $(CFLAGS) $< -o $@ -c

pron.o: \
	prun.c \
	prun.h
