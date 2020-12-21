# NAME: Seungwon Kim
# EMAIL: haleykim@g.ucla.edu
# ID: 405111152

# Only Error Messages Generated
.SILENT:

# Define Variables
CC = gcc
CFLAGS = -Wall -Wextra
TARGET = lab0
FILES = lab0.c Makefile README backtrace.png breakpoint.png

# Build Default Executable
all: $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

# Run Smokechecks
check: all runTests
runTests: regularCase bogusCase inputFirstCase outputFirstCase inputCase outputCase catchCase segCatchCase catchSegCase 

# ./lab0 --input input.txt --output output.txt
# should be successful, exit code 0
regularCase:
	echo "Contents of Input File" > input.txt \
	-./lab0 --input input.txt --output output.txt 2> /dev/null \
	if [ $$? -ne 0 ] \
	then \
		echo "Error: program did not successfully complete and exit with code 0" > logfile.txt \
		cat logfile.txt \
	fi
	rm -f *.txt logfile.txt
# &> redirects fd1 and fd2 to rhs

# ./lab0 --bogus
# should be unsuccessful, exit code 1
bogusCase:
	-./lab0 --bogus 2> /dev/null \
	if [ $$? -ne 1 ] \
	then \
		echo "Error: --bogus did not end with exit code 1" > logfile.txt \
		cat logfile.txt \
	fi
	rm -f *.txt

# ./lab0 --input invalid.txt --output invalid.txt
# should be unsuccessful, exit code 2
inputFirstCase:
	touch bar.txt
	chmod 000 bar.txt
	-./lab0 --input foo.txt --output bar.txt 2> /dev/null \
	if [ $$? -ne 2 ] \
	then \
		echo "Error: --input invalid.txt coming before --output invalid.txt did not end with exit code 2" > logfile.txt \
		cat logfile.txt \
	fi
	rm -f *.txt

# ./lab0 --output invalid.txt --input invalid.txt
# should be unsuccessful, exit code 3
outputFirstCase:
	touch bar.txt
	chmod 000 bar.txt
	-./lab0 --input foo.txt --output bar.txt 2> /dev/null \
	if [ $$? -ne 2 ] \
	then \
		echo "Error: --input invalid.txt coming before --output invalid.txt did not end with exit code 2" > logfile.txt \
		cat logfile.txt \
	fi
	rm -f *.txt

# ./lab0 --input invalid.txt
# should be unsuccessful, exit code 2
inputCase:
	-./lab0 --input foo.txt 2> /dev/null \
	if [ $$? -ne 2 ] \
	then \
		echo "Error: invalid filename for --input did not end with exit code 2" > logfile.txt \
		cat logfile.txt \
	fi
	rm -f *.txt

# ./lab0 --output invalid.txt
# should be unsuccessful, exit code 3
outputCase:
	touch bar.txt
	chmod 000 bar.txt
	-./lab0 --output bar.txt 2> /dev/null \
	if [ $$? -ne 3 ] \
	then \
		echo "Error: invalid filename for --output did not end with exit code 3" > logfile.txt \
		cat logfile.txt \
	fi
	rm -f *.txt

# ./lab0 --catch
# should be successful, exit code 0
catchCase:
	-./lab0 --catch 2> /dev/null \
	if [ $$? -ne 0 ] \
	then \
		echo "Error: --catch did not successfully end with exit code 0" > logfile.txt \
		cat logfile.txt \
	fi
	rm -f *.txt

# ./lab0 --segfault --catch
# should be successful, exit code 4
segCatchCase:
	-./lab0 --segfault --catch 2> /dev/null \
	if [ $$? -ne 4 ] \
	then \
		echo "Error: did not catch segfault and end with exit code 4" > logfile.txt \
		cat logfile.txt \
	fi
	rm -f *.txt

# ./lab0 --catch --segfault
# should be successful, exit code 4
catchSegCase:
	-./lab0 --catch --segfault 2> /dev/null \
	if [ $$? -ne 4 ] \
	then \
		echo "Error: did not catch segfault and end with exit code 4" > logfile.txt \
		cat logfile.txt \
	fi
	rm -f *.txt

# make clean for a fresh start
clean:
	rm -R -f lab0 lab0-405111152.tar.gz

# Create tarball
dist:
	tar -czf lab0-405111152.tar.gz $(FILES)
