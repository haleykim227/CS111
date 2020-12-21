# NAME: Seungwon Kim
# EMAIL: haleykim@g.ucla.edu
# ID: 405111152

# Only Error Messages Generated
.SILENT:

# Define Variables
CC = gcc
CFLAGS = -Wall -Wextra -lmraa -lm -std=c99 -g -lssl -lcrypto
TARGET1 = lab4c_tcp
TARGET2 = lab4c_tls
FILES = lab4c_tls.c lab4c_tcp.c Makefile README

# Build Default Executable
all:
	$(CC) $(CFLAGS) -o $(TARGET1) $(TARGET1).c
	$(CC) $(CFLAGS) -o $(TARGET2) $(TARGET2).c

# make clean for a fresh start
clean:
	rm -R -f lab4c_tcp lab4c_tls lab4c-405111152.tar.gz

# Create Tarball
dist:
	tar -czf lab4c-405111152.tar.gz $(FILES)
