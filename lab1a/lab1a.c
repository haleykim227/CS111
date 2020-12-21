// NAME: Seungwon Kim
// EMAIL: haleykim@g.ucla.edu
// ID: 405111152

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

// Definition of Variables
struct termios original_termios;
struct termios new_termios;
int getopt_long_ret = 0;
int long_index = 0;
int shell_flag = 0;
char shell_arg[256];
int pipe_ts[2];
int pipe_fs[2];
pid_t pid_child;
int pipe_closed_flag = 0; // 0 if open, 1 if closed
int exit_flag = 0; // 0 if not ready, 1 if ready

// List of Helper Functions
void save_termios();
void set_termios();
void reset_termios();
void terminal_echo();
void create_pipes();
void signal_handler(int signum);
void console_to_child();
void child_to_console();

// Helpter Function Implementations

// Save Original Termios Attributes
void save_termios() {
  int tcgetattr_ret = tcgetattr(STDIN_FILENO, &original_termios);
  // Error Checking tcgetattr()
  if (tcgetattr_ret < 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Could not retrieve terminal attributes\n");
    exit(1);
  }
}

// Set New Termios Attributes
void set_termios() {
  save_termios(); // insurance in case main forgets to run save_termios()
  int tcgetattr_ret = tcgetattr(STDIN_FILENO, &new_termios);
  // Error Checking tcgetattr()
  if (tcgetattr_ret < 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Could not retrieve terminal attributes\n");
    exit(1);
  }
  new_termios.c_iflag = ISTRIP;
  new_termios.c_oflag = 0;
  new_termios.c_lflag = 0;
  // Apply New Attributes
  int tcsetattr_ret = tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
  // Error Checking tcsetattr()
  if (tcsetattr_ret < 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Could not set terminal attributes\n");
    exit(1);
  }
  atexit(reset_termios);
}

// Reset Termios Attributes to Original
void reset_termios() {
  int tcsetattr_ret = tcsetattr(0, TCSANOW, &original_termios);
  // Error Checking tcsetattr()
  if (tcsetattr_ret < 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Could not reset terminal attributes\n");
    exit(1);
  }
}

// Reading and Writing char-by-char, echo
void terminal_echo() {
  // Initial Read from Terminal
  const int BYTE_COUNT = 8;
  char* buffer = (char*)malloc(BYTE_COUNT*sizeof(char));
  // Read, Parse Through Read, Read Again
  ssize_t bytes_read;
  while (1) {
    bytes_read = read(STDIN_FILENO, buffer, BYTE_COUNT);
    // Error Checking read()
    if (bytes_read < 0) {
      fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
      fprintf(stderr, "Could not read char from terminal\n");
      exit(1);
    } 
    // Go through the 8 bytes of buffer one-by-one
    for (int i = 0; i < bytes_read; i++) {
      char temp_char = buffer[i];
      char temp[2];
      switch(buffer[i]) {
      case 0x04: // ^D, EOF
	temp[0] = '^';
	temp[1] = 'D';
	ssize_t wr_ret2 = write(STDOUT_FILENO, temp, 2);
	// Error Checking write()
	if (wr_ret2 < 0) {
	  fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	  fprintf(stderr, "Could not write ^D\n");
	  exit(1);
	}
	reset_termios();
	exit(0);
	break;
      case '\r': // \r or \n
      case '\n':
	temp[0] = '\r';
	temp[1] = '\n';
	ssize_t wr_ret3 = write(STDOUT_FILENO, temp, 2);
	// Error Checking write()
	if (wr_ret3 < 0) {
	  fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	  fprintf(stderr, "Could not write carriage return and newline\n");
	  exit(1);
	}
	break;
      default:; // write char out to terminal
	ssize_t wr_ret4 = write(STDOUT_FILENO, &temp_char, 1);
	// Error Checking write()
	if (wr_ret4 < 0) {
	  fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	  fprintf(stderr, "Could not write char to terminal\n");
	  exit(1);
	}
	break;
      }
    }
    memset(buffer, 0, bytes_read);
  }
}

// Creating Two Uni-Directional Pipes
void create_pipes() {
  // First pipe, from lab1a to shell
  int pipe_ts_ret = pipe(pipe_ts);
  // Error Checking pipe()
  if (pipe_ts_ret < 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Could not create pipe to shell\n");
    exit(1);
  }
  // Second pipe, from shell to lab1a
  int pipe_fs_ret = pipe(pipe_fs);
  // Error Checking pipe()
  if (pipe_fs_ret < 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Could not create pipe from shell\n");
    exit(1);
  }
}

// Handling SIGPIPE from poll()
void signal_handler(int signum) {
  if (signum == SIGPIPE) {
    exit_flag = 1;
  }
}

// Process input from keyboard + forward to shell + echo to screen
void console_to_child() {
 // Initial Read from Terminal
  const int BYTE_COUNT = 8;
  char* buffer = (char*)malloc(BYTE_COUNT*sizeof(char));
  // Read, Parse Through Read, Read Again
  ssize_t bytes_read;
  bytes_read = read(STDIN_FILENO, buffer, BYTE_COUNT);
  // Error Checking read()
  if (bytes_read < 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Could not read char from terminal\n");
    exit(1);
  } 
  // Go through the 8 bytes of buffer one-by-one
  for (int i = 0; i < bytes_read; i++) {
    char temp_char = buffer[i];
    char temp[2];
    switch(buffer[i]) {
    case 0x03: // ^C, interrupt, send SIGINT to child
      temp[0] = '^';
      temp[1] = 'C';
      ssize_t wr_ret1 = write(STDOUT_FILENO, temp, 2);
      if (wr_ret1 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write to terminal\n");
	exit(1);
      }
      int kill_ret = kill(pid_child, SIGINT);
      // Error Checking kill()
      if (kill_ret < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not send SIGINT to pid_child\n");
	exit(1);
      }
      break;
    case 0x04: // ^D, EOF, do not exit program at ^D, only close pipe to shell
      temp[0] = '^';
      temp[1] = 'D';
      ssize_t wr_ret2 = write(STDOUT_FILENO, temp, 2);
      // Error Checking write()
      if (wr_ret2 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write ^D\n");
	exit(1);
      }
      if (!pipe_closed_flag) {
	close(pipe_ts[1]);
	pipe_closed_flag = 1;
      }
      break;
    case '\r': // \r or \n
    case '\n':
      temp[0] = '\r';
      temp[1] = '\n';
      ssize_t wr_ret3 = write(STDOUT_FILENO, temp, 2);
      ssize_t wr_ret4 = write(pipe_ts[1], &temp[1], 1);
      // Error Checking write()
      if (wr_ret3 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write carriage return and newline\n");
	exit(1);
      }
      if (wr_ret4 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write newline to child\n");
	exit(1);
      }
      break;
    default:; // write char out to terminal
      ssize_t wr_ret5 = write(STDOUT_FILENO, &temp_char, 1);
      ssize_t wr_ret6 = write(pipe_ts[1], &temp_char, 1);
      // Error Checking write()
      if (wr_ret5 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write char to terminal\n");
	exit(1);
      }
      if (wr_ret6 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write char to child\n");
	exit(1);
      }
      break;
    }
  }
  memset(buffer, 0, bytes_read);
} 

// Process input from child + forward to terminal 
void child_to_console() {
 // Initial Read from Child
  const int BYTE_COUNT = 256;
  char* buffer = (char*)malloc(BYTE_COUNT*sizeof(char));
  // Read, Parse Through Read, Read Again
  ssize_t bytes_read;
  bytes_read = read(pipe_fs[0], buffer, BYTE_COUNT);
  // Error Checking read()
  if (bytes_read < 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Could not read char from terminal\n");
    exit(1);
  } 
  // Go through the 8 bytes of buffer one-by-one
  for (int i = 0; i < bytes_read; i++) {
    char temp_char = buffer[i];
    char temp[2];
    switch(buffer[i]) {
    case 0x04: // ^D, EOF, do not exit program at ^D, only close pipe to shell
      temp[0] = '^';
      temp[1] = 'D';
      ssize_t wr_ret1 = write(STDOUT_FILENO, temp, 2);
      // Error Checking write()
      if (wr_ret1 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write ^D\n");
	exit(1);
      }
      exit_flag = 1;
      break;
    case '\n':
      temp[0] = '\r';
      temp[1] = '\n';
      ssize_t wr_ret2 = write(STDOUT_FILENO, temp, 2);
      // Error Checking write()
      if (wr_ret2 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write carriage return and newline\n");
	exit(1);
      }
      break;
    default:; // write char out to terminal
      ssize_t wr_ret3 = write(STDOUT_FILENO, &temp_char, 1);
      // Error Checking write()
      if (wr_ret3 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write char to terminal\n");
	exit(1);
      }
      break;
    }
  }
  memset(buffer, 0, bytes_read);
}

// Main Routine
int main (int argc, char *argv[]) {
  // Parsing Options and Arguments
  while (1) {
    static struct option long_options[] = {{"shell", required_argument, 0, 's'},
					   {0, 0, 0, 0}};
    getopt_long_ret = getopt_long(argc, argv, "s:", long_options, &long_index);

    // Break once parking is complete
    if (getopt_long_ret == -1)
      break;

    // Setting variables and flags based on options and arguments
    switch(getopt_long_ret) {
    case 's':
      shell_flag = 1;
      strcpy(shell_arg, optarg);
      break;
    case '?':
      if (strcmp(argv[optind-1], "--shell") == 0) {
	fprintf(stderr, "--shell: missing argument\n");
	exit(1);
      }
      else {
	fprintf(stderr, "./lab1a: unrecognized option '%s'\n", argv[optind-1]);
	fprintf(stderr, "Usage: ./lab1a [--shell] programName\n");
	exit(1);
      }
      break;
    }
  }

  // Termios
  save_termios();
  set_termios();

  // Actions based on stored variables and flags

  // When --shell flag is used
  if (shell_flag) {
    signal(SIGPIPE, signal_handler);
    // Creating Pipes to and from shell
    create_pipes();
    // Creating Child Process with 4 New FDs
    pid_child = fork();
    // Error Checking fork()
    if (pid_child < 0) {
      fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
      fprintf(stderr, "Could not create child process\n");
      exit(1);
    }
    // Inside Child Process
    else if (pid_child == 0) {
      // Close Unused Pipe Ends
      close(pipe_ts[1]); // close to_shell_out
      close(pipe_fs[0]); // close from_shell_in
      // Connect Pipe from lab1a to child
      close(STDIN_FILENO); // close fd 1
      dup(pipe_ts[0]); // move to_shell_in to fd 1
      close(pipe_ts[0]); // close redundancy
      // Connect Pipe from child to lab1a
      close(STDOUT_FILENO); // close fd 2
      dup(pipe_fs[1]); // move from_shell_out to fd 2
      close(STDERR_FILENO); // close fd 3
      dup(pipe_fs[1]); // move from_shell_out to fd 3
      close(pipe_fs[1]); // close redundancy
      // Make Child Run Given Program
      int execl_ret = execl("/bin/bash", "sh", (char *) NULL);
      // Error Checking execl()
      if (execl_ret < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not execute on child process\n");
	exit(1);
      }
    }
    // Inside Parent Process
    else if (pid_child > 0) {
      close(pipe_ts[0]); // close to_shell_in
      close(pipe_fs[1]); // close from_shell_out
      int poll_ret;
      int status_child;
      struct pollfd inputs[] = {{STDIN_FILENO, POLLIN, 0},
				{pipe_fs[0], POLLIN, 0}};
      while (1) {
	poll_ret = poll(inputs, 2, -1); // -1 is like infinity
	// Error Checking poll()
	if (poll_ret < 0) {
	  fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	  fprintf(stderr, "Could not poll input channels\n");
	  exit(1);
	}
	// Some events or event has occurred
	if (poll_ret > 0) {
	  if (inputs[0].revents & POLLIN)
	    console_to_child();
	  if (inputs[1].revents & POLLIN)
	    child_to_console();
	  if (inputs[0].revents & (POLLERR | POLLHUP))
	    break;
	  if (inputs[1].revents & (POLLERR | POLLHUP))
	    break;
	}
	if (exit_flag)
	  break;
      }
      // When poll concludes, close pipe
      if (!pipe_closed_flag) {
	int close_ret = close(pipe_ts[1]);
	// Error Checking close()
	if (close_ret < 0) {
	  fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	  fprintf(stderr, "Could not close pipe to shell\n");
	  exit(1);
	}
      }
      pid_t waitpid_ret = waitpid(pid_child, &status_child, 0);
      // Error Checking waitpid()
      if (waitpid_ret < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));	   fprintf(stderr, "Could not receive exit status of child\n");
	exit(1);
      }
      fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status_child), WEXITSTATUS(status_child));
      exit(0);
    }
  }

  // Without --shell argument,
  // echo back what user inputs character by character
  else {
    terminal_echo();
  }
}
