# NAME: Seungwon Kim,Shri Narayana
# EMAIL: haleykim@g.ucla.edu,shrinikethn@ucla.edu
# ID: 405111152,505313060

# Only Error Messages Generated
.SILENT:

# Define Variables
CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = lab3a
FILES = lab3a.c Makefile README ext2_fs.h

# Build Default Executable
all: $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

# make clean for a fresh start
clean:
	rm -R -f lab3a lab3a-405111152.tar.gz

# Create tarball
dist:
	tar -czf lab3a-405111152.tar.gz $(FILES)
