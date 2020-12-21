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
}
