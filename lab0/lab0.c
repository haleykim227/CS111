// NAME: Seungwon Kim
// EMAIL: haleykim@g.ucla.edu
// ID: 405111152

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char *argv[]) {

  // Definition of Variables
  int a = 0;
  int long_index = 0;
  int input_flag = 0;
  int output_flag = 0;
  int segfault_flag = 0;
  int catch_flag = 0;
  int input_ind = 0;
  int output_ind = 0;
  char input_arg[25];
  char output_arg[25];

  // Helper Functions
  void create_segfault() {
    char* ptr = NULL;
    *ptr = 'a';
    
  }

  void signal_handler(int signum) {
    if (signum == SIGSEGV) {
      fprintf(stderr, "Segmentation fault: caught and handled\n");
      _Exit(4);
    }
  }
  
  // Parsing Options and Arguments
  while (1) {
    static struct option long_options[] = {
					   {"input", required_argument, 0, 'i'},
					   {"output", required_argument, 0, 'o'},
					   {"segfault", no_argument, 0, 's'},
					   {"catch", no_argument, 0, 'c'},
					   {0, 0, 0, 0}};
    a = getopt_long(argc, argv, "i:o:sc", long_options, &long_index);

    // Break once parsing is complete
    if (a == -1)
      break;

    // Setting variables and flags based on options and arguments
    switch (a) {
    case 'i':
      input_flag = 1;
      input_ind = optind - 1;
      strcpy(input_arg, optarg);
      break;
    case 'o':
      output_flag = 1;
      output_ind = optind - 1;
      strcpy(output_arg, optarg);
      break;
    case 's':
      segfault_flag = 1;
      break;
    case 'c':
      catch_flag = 1;
      break;
    case '?':
      if (strcmp(argv[optind-1], "--input") == 0)
	printf("--input: missing argument\n");
      else if (strcmp(argv[optind-1], "--output") == 0)
	printf("--output: missing argument\n");
      else {
	printf("./lab0: unrecognized option '%s'\n", argv[optind-1]);
	printf("Usage: ./lab0 [--input] infilename [--output] outfilename [--segfault] [--catch]\n");
	_Exit(1);
      }
      break;
    }
  }

  // Actions based on stored variables and flags

  // Definition of Variables
  int i_o1_ret = 0;
  int o_o1_ret = 0;
  
  // When --input is used
  if (input_flag) {
    i_o1_ret = open(input_arg, O_RDONLY);
    // If there are no errors opening file, carry on with i/o
    if (i_o1_ret != -1) {
      close(0);
      dup(3);
      close(3);
    }
  }

  // When --output is used
  if (output_flag) {
    o_o1_ret = open(output_arg, O_CREAT|O_RDWR|O_TRUNC, 0666);
    // If there are no errors creating file, carry on with i/o
    if (o_o1_ret != -1) {
      close(1);
      dup(3);
      close(3);
    }
  }

  // If both --input and --output caused errors
  if (i_o1_ret == -1 && o_o1_ret == -1) {
    // If --input comes before --output
    if (input_ind < output_ind) {
      fprintf(stderr, "%s: %s\n", input_arg, strerror(2));
      _Exit(2);
    }
    // If --output comes before --input
    if (output_ind < input_ind) {
      fprintf(stderr, "%s: %s\n", output_arg, strerror(errno));
      _Exit(3);
    }
  }
  // If only --input caused error
  if (i_o1_ret == -1 && o_o1_ret != -1) {
    fprintf(stderr, "%s: %s\n", input_arg, strerror(errno));
    _Exit(2);
  }
  // If only --output caused error
  if (i_o1_ret != -1 && o_o1_ret == -1) {
    fprintf(stderr, "%s: %s\n", output_arg, strerror(errno));
    _Exit(3);
  }

  // If --catch option is used
  if (catch_flag) {
    signal(SIGSEGV, signal_handler);
  }

  // If --segfault option is used
  if (segfault_flag) {
    create_segfault();
  }
  
  // Read in and write out one byte at a time
  char* buffer = (char*)malloc(sizeof(char)); // create buffer for 1 char
  ssize_t bytes_read = read(0, buffer, 1); // read in 1 char from fd0
  while (bytes_read > 0) {
    write(1, buffer, 1); // write 1 char to fd1
    bytes_read = read(0, buffer, 1); // read next char from fd0
    // if input file over, bytes_read is 0, exits loop
  }
  free(buffer); // once task is done, free the 1 char buffer
  _Exit(0);
}


