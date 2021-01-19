# NAME: Seungwon Kim
# EMAIL: haleykim@g.ucla.edu
# ID: 405111152

# Only Error Messages Generated
.SILENT:

# Define Variables
CC = gcc
CFLAGS = -Wall -Wextra -lmraa -lm -std=c99 -g
TARGET = lab4b
FILES = lab4b.c Makefile README smoketest.sh

# Build Default Executable
all: $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

# Automated smoke-test
check: all dist
	-rm -f log.txt
	chmod 744 smoketest.sh
	./smoketest.sh
	rm -f log.txt

# Make Clean
clean:
	rm -R -f lab4b lab4b-405111152.tar.gz

# Create Tarball
dist:
	tar -czf lab4b-405111152.tar.gz $(FILES)
