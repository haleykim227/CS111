# NAME: Seungwon Kim
# EMAIL: haleykim@g.ucla.edu                                                             
# ID: 405111152                                                                          

# Only Error Messages Generated                                                          
.SILENT:

# Define Variables
CC = gcc
CFLAGS = -Wall -Wextra -lz
TARGET1 = lab1b-client
TARGET2 = lab1b-server
FILES = lab1b-client.c lab1b-server.c Makefile README

# Build Default Executable (client and server)
all:
	gcc -Wall -Wextra -lz -o lab1b-client lab1b-client.c
	gcc -Wall -Wextra -lz -o lab1b-server lab1b-server.c

# Build Only Client
client:
	$(CC) $(CFLAGS) -o $(TARGET1) $(TARGET1).c

# Build Only Server
server:
	$(CC) $(CFLAGS) -o $(TARGET2) $(TARGET2).c

# Make Clean                                                                             
clean:
	rm -R -f lab1b-client lab1b-server lab1b-405111152.tar.gz

# Create Tarball                                                                        
dist:
	tar -czf lab1b-405111152.tar.gz $(FILES)
