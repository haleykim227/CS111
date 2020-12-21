# NAME: Haley Kim,Shri Narayana                                                         
# EMAIL: haleykim@g.ucla.edu,shrinikethn@ucla.edu                                       
# ID: 405111152,505313060 

# Only Error Messages Generated
.SILENT:

# Define Variables
FILES = lab3b.py Makefile README

# Build Default Executable
all:
	rm -f lab3b
	ln -s lab3b.py lab3b
	chmod 744 lab3b
	echo "make all: Successful Compilation"

# Make Clean
clean:
	rm -R -f lab3b lab3b-405111152.tar.gz

# Create Tarball
dist:
	tar -czf lab3b-405111152.tar.gz $(FILES)
