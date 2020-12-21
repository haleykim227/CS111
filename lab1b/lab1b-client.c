// NAME: Seungwon Kim
// EMAIL: haleykim@g.ucla.edu
// ID: 405111152

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <zlib.h>

// Definition of Variables
struct termios original_termios;
struct termios new_termios;
int getopt_long_ret = 0;
int long_index = 0;
int port_flag = 0;
int log_flag = 0;
int compress_flag = 0;
int port_arg;
char log_arg[256];
z_stream stdin_to_server;
z_stream server_to_stdout;
struct sockaddr_in server_address;
struct hostent* server;
int sockfd;
int logfd;

// List of Helper Functions
void save_termios();
void set_termios();
void reset_termios();

// Helper Function Implementations

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

// Create Compression Stream, stdin to server
void compression_init() {
  // stdin_to_server z_stream
  stdin_to_server.zalloc = Z_NULL;
  stdin_to_server.zfree = Z_NULL;
  stdin_to_server.opaque = Z_NULL;
  int deflateInit_ret = deflateInit(&stdin_to_server, Z_DEFAULT_COMPRESSION);
  // Error Checking deflateInit()
  if (deflateInit_ret != Z_OK) {
    fprintf(stderr, "Error: deflateInit() failure, cannot create compression stream.\n");
    exit(1);
  }  
}

// Create Decompression Stream, server to stdout
void decompression_init() {
  // server_to_stdout z_stream
  server_to_stdout.zalloc = Z_NULL;
  server_to_stdout.zfree = Z_NULL;
  server_to_stdout.opaque = Z_NULL;
  int inflateInit_ret = inflateInit(&server_to_stdout);
  // Error Checking inflateInit()
  if (inflateInit_ret != Z_OK) {
    fprintf(stderr, "Error: inflateInit() failure, cannot create decompression stream.\n");
    exit(1);
  }
}

// Create Socket & Check Validity
void create_socket() {
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  // Error Checking socket()
  if (sockfd < 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Could not create socket using socket()\n");
    exit(1);
  }
  // Checking Validity of host using gethostbyname
  // Recommended by TA Dharm
  server = gethostbyname("localhost");
  if (server == NULL) {
    fprintf(stderr, "Error: check gethostbyname(), no such host exists\n");
    exit(1);
  }
  // TA Dharm's Sample Code
  memset((char*) &server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port_arg);
  memcpy((char*) &server_address.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
}

// Connect to Socket
void connect_socket() {
  int connect_ret = connect(sockfd, (struct sockaddr*) &server_address, sizeof(server_address));
  // Error Checking connect()
  if (connect_ret < 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Could not connect to sockfd using connect()\n");
    exit(1);
  }
}

// Read from STDIN Write to Server
void read_stdin_write_server() {
  // Initial Read from Terminal
  const int BYTE_COUNT = 256;
  char compression_buffer[256];
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
  // Go through the 256 bytes of buffer one-by-one
  for (int i = 0; i < bytes_read; i++) {
    char temp_char = buffer[i];
    char temp[2];
    switch(temp_char) {
    case '\r': // \r or \n
    case '\n':
      compression_buffer[i] = '\n';
      temp[0] = '\r';
      temp[1] = '\n';
      ssize_t wr_ret1 = write(STDOUT_FILENO, temp, 2);
      // Error Checking write()
      if (wr_ret1 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write carriage return and newline to STDOUT\n");
	exit(1);
      }
      // Only Pass Along Directly to Socket if --compress Option Not Unused
      if (compress_flag == 0) {
	ssize_t wr_ret2 = write(sockfd, &temp[1], 1);
	// Error Checking write()
	if (wr_ret2 < 0) {
	  fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	  fprintf(stderr, "Could not write newline to socket\n");
	  exit(1);
	}
      }
      break;
    default:;
      compression_buffer[i] = temp_char;
      ssize_t wr_ret3 = write(STDOUT_FILENO, &temp_char, 1);
      // Error Checking write()
      if (wr_ret3 < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not write to STDOUT\n");
	exit(1);
      }
      // Only Pass Along Directly to Socket if --compress Option Unused
      if (compress_flag == 0) {
	ssize_t wr_ret4 = write(sockfd, &temp_char, 1);
	// Error Checking write()
	if (wr_ret4 < 0) {
	  fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	  fprintf(stderr, "Could not write raw data to socket\n");
	  exit(1);
	}
      }
      break;
    }
  }
  // --compress Option Used
  if (compress_flag) {
    char compression_temp [256];
    memcpy(compression_temp, compression_buffer, bytes_read);
    stdin_to_server.avail_in = bytes_read;
    stdin_to_server.next_in = (Bytef*) compression_temp;
    stdin_to_server.avail_out = 256;
    stdin_to_server.next_out = (Bytef*) compression_buffer;
    // Compress While More Bytes Available
    do {
      int deflate_ret = deflate(&stdin_to_server, Z_SYNC_FLUSH);
      // Error Checking deflate()
      if (deflate_ret == Z_STREAM_ERROR) {
	fprintf(stderr, "Inconsistent Stream State: %s", stdin_to_server.msg);
	exit(1);
      }
    } while (stdin_to_server.avail_in > 0);
    int wr_ret5 = write(sockfd, &compression_buffer, bytes_read);
    // Error Checking write()
    if (wr_ret5 < 0) {
      fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
      fprintf(stderr, "Could not write compressed data to socket\n");
      exit(1);
    }
  }
  // --log Option Used
  if (log_flag) {
    //int dprintf_ret1 = dprintf(logfd, "SENT %d bytes: ", bytes_read);
    // Error Checking dprintf()
    //if (dprintf_ret1 < 0) {
    //fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    //fprintf(stderr, "Could not write to logfile\n");
    //}
    int dprintf_ret2 = write(logfd, &compression_buffer, bytes_read);
    // Error Checking dprintf()
    if (dprintf_ret2 < 0) {
      fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
      fprintf(stderr, "Could not write to logfile\n");
    }
    char newline[1];
    newline[0] = '\n';
    int dprintf_ret3 = dprintf(logfd, &newline[0], sizeof(char));
    // Error Checking dprintf()
    if (dprintf_ret3 < 0) {
      fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
      fprintf(stderr, "Could not write newline to logfile\n");
      exit(1);
    }
  }
  memset(buffer, 0, bytes_read);
}

// Read from Server Write to STDOUT
void read_server_write_stdout() {
}

// Main Routine
int main(int argc, char *argv[]) {
  // Parsing Options and Arguments                                                     
  while (1) {
    static struct option long_options[] = {{"port", required_argument, 0, 'p'},
					   {"log", required_argument, 0, 'l'},
					   {"compress", no_argument, 0, 'c'},
                                           {0, 0, 0, 0}};
    getopt_long_ret = getopt_long(argc, argv, "p:l:c", long_options, &long_index);

    // Break once parking is complete                                                    
    if (getopt_long_ret == -1)
      break;
    
    // Setting variables and flags based on options and arguments                        
    switch(getopt_long_ret) {
    case 'p':
      port_flag = 1;
      port_arg = atoi(optarg);
      break;
    case 'l':
      log_flag = 1;
      strcpy(log_arg, optarg);
      // Create Log File
      logfd = creat(optarg, S_IRWXU);
      // Error Checking creat()
      if (logfd < 0) {
	fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
	fprintf(stderr, "Could not createe logfile using creat()\n");
	exit(1);
      }
      break;
    case 'c':
      compress_flag = 1;
      break;
    case '?':
      if (strcmp(argv[optind-1], "--port") == 0) {
        fprintf(stderr, "--port: missing argument\n");
        exit(1);
      }
      else if (strcmp(argv[optind-1], "--log") == 0) {
	fprintf(stderr, "--log: missing argument\n");
	exit(1);
      }
      else {
        fprintf(stderr, "./lab1b-client: unrecognized option '%s'\n", argv[optind-1]);
        fprintf(stderr, "Usage: ./lab1b-client [--port] portNumber [--log] logFileName [--compress]\n");
        exit(1);
      }
      break;
    }
  }

  // Termios Altering
  save_termios();
  set_termios();

  // Create Socket
  create_socket();

  // Connect to Socket
  connect_socket();

  // Read from STDIN and Socket to Server
  struct pollfd inputs[] = {{STDIN_FILENO, POLLIN, 0}, // stdin
			    {sockfd, POLLIN, 0}}; // socket
  int poll_ret;
  while(1) {
    poll_ret = poll(inputs, 2,-1);
    // Error Checking poll()
    if (poll_ret < 0) {
      fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
      fprintf(stderr, "Could not poll input channels using poll()\n");
      exit(1);
    }
    if (poll_ret > 0) {
      if (inputs[0].revents & POLLIN) {
	// read from stdin write to socket
	read_stdin_write_server();
      }
      if (inputs[1].revents & POLLIN) {
	// read from socket write to stdout
	read_server_write_stdout();
      }
      // Input from STDIN disconnected, error
      if (inputs[0].revents & (POLLERR | POLLHUP)) {
	fprintf(stderr, "Error: POLLERR or POLLHUP from STDIN\n");
	exit(1);
      }
      // Socket Closed, Normal Exit Conditions
      if (inputs[1].revents & (POLLERR | POLLHUP)) {
	close(sockfd);
	exit(0);
      }
    }
  }
}
