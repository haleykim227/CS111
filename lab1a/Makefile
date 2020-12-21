# NAME: Seungwon Kim
# EMAIL: haleykim@g.ucla.edu
# ID: 405111152

# Only Error Messages Generated
.SILENT:

# Define Variables
CC = gcc
CFLAGS = -Wall -Wextra
TARGET = lab1a
FILES = lab1a.c Makefile README

# Build Default Executable
all: $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

# Make Clean
clean:
	rm -R -f lab1a lab1a-405111152.tar.gz

# Create Tarball
dist:
	tar -czf lab1a-405111152.tar.gz $(FILES)
