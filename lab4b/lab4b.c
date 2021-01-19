// NAME: Seungwon Kim
// EMAIL: haleykim@g.ucla.edu
// ID: 405111152

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <mraa.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

// Definition of Global Variables
int long_index = 0; // for getopt_long_ret()
int period_flag = 0;
int scale_flag = 0;
int log_flag = 0;
int report_flag = 1;
double period_arg = 1.0; // default to 1 second
char scale_arg = 'F'; // should be only 1 char long
char log_arg[256] = {'\0'}; // strlen would be 0
mraa_aio_context temp_sensor;
mraa_gpio_context button;

// List of Helper Functions
float get_temp();
void init_contexts();
void init_buffers();
void input_handling();

// Helper Function Implementation
float get_temp() {
	// Beta-Value and Resistance
	const int B = 4275;
	const int R0 = 100000;
	// Reading in temp from Thermistor
	int temp_voltage = mraa_aio_read(temp_sensor);
	// Calculation from Voltage to C
	float R = 1023.0/((float)temp_voltage)-1.0;
	R = R0*R;
	float temp_c = 1.0/(log(R/R0)/B+1/298.15)-273.15;
	if (scale_arg == 'C')
		return temp_c;
	else
		return (temp_c * 9)/5 + 32;
}

// Initialize Sensor Contexts, Declare Direction
void init_contexts() {
	temp_sensor = mraa_aio_init(1); // 1 is A0/A1
	button = mraa_gpio_init(60); // 60 is GPIO_50
	// Error Checking Context Initialization
	if (temp_sensor == NULL){
		fprintf(stderr, "Error: could not initialize A0/A1\n");
		mraa_deinit();
		exit(1);
	}
	if (button == NULL){
		fprintf(stderr, "Error: could not initialize GPIO_50\n");
		mraa_deinit();
		exit(1);
	}
	// Declare that Button is Used for Input
	mraa_gpio_dir(button, MRAA_GPIO_IN); // analog does not need direction
}

// Handling Input from STDIN
void input_handling(char* mini_buffer) {
	if (strcmp(mini_buffer, "SCALE=F") == 0) {
		scale_arg = 'F';
		if (log_flag) {
			dprintf(4, "SCALE=F\n");
		}
	}
	else if (strcmp(mini_buffer, "SCALE=C") == 0) {
		scale_arg = 'C';
		if (log_flag){
			dprintf(4, "SCALE=C\n");
		}
	}
	else if(strncmp(mini_buffer, "PERIOD=", sizeof(char)*7) == 0) {
		period_arg = atoi(mini_buffer+7);
		if (log_flag) {
			dprintf(4, "%s\n", mini_buffer);
		}
	}
	else if(strcmp(mini_buffer, "STOP") == 0) {
		report_flag = 0;
		if (log_flag) {
			dprintf(4, "STOP\n");
		}
	}
	else if(strcmp(mini_buffer, "START") == 0) {
		report_flag = 1;
		if (log_flag) {
			dprintf(4, "START\n");
		}
	}
	else if((strncmp(mini_buffer, "LOG", sizeof(char)*3) == 0)) {
		if (log_flag) {
			dprintf(4, "%s\n", mini_buffer);
		}
	}
	if(strcmp(mini_buffer, "OFF") == 0) {
		struct timeval off_time;
		gettimeofday(&off_time, NULL);
		struct tm* log_time;
		log_time = localtime(&(off_time.tv_sec));
		if (report_flag) {
			fprintf(stdout, "%02d:%02d:%02d SHUTDOWN\n", log_time->tm_hour, 
				log_time->tm_min, log_time->tm_sec);
		}
		if (log_flag) {
			dprintf(4, "OFF\n");
			dprintf(4, "%02d:%02d:%02d SHUTDOWN\n", log_time->tm_hour, 
				log_time->tm_min, log_time->tm_sec);
		}
		exit(0);
	}
}

// Main Routine
int main (int argc, char *argv[]) {

	// Parsing Options and Arguments
	while (1) {
		static struct option long_options[] = {{"period", required_argument, 0, 'p'},
						 {"scale", required_argument, 0, 's'},
						 {"log", required_argument, 0, 'l'},
						 {0, 0, 0, 0}};

		int getopt_long_ret = getopt_long(argc, argv, "p:s:l:", long_options, &long_index);

		// Break once parking is complete
		if (getopt_long_ret == -1)
			break;

		// Setting variables and flags based on options and arguments
		switch(getopt_long_ret) {
		case 'p':
			period_flag = 1;
			period_arg = atof(optarg);
			break;
		case 's':
			scale_flag = 1;
			if (strlen(optarg) > 1) {
			 fprintf(stderr, "Error: --scale argument not single char\n");
			 exit(1);
			}
			else
			 strcpy(&scale_arg, optarg);
			// if not C or F, invalid argument
			if ((scale_arg != 'C') && (scale_arg != 'F')) {
				fprintf(stderr, "Error: --scale invalid argument\n");
				exit(1);
			}
			break;
		case 'l':
			log_flag = 1;
			strcpy(log_arg, optarg);
			// open/create logfile
			int fd = open(log_arg, O_CREAT|O_TRUNC|O_WRONLY);
			// for some reason, fd will consistently be 4
			// Error Checking open()
			if (fd == -1) {
				fprintf(stderr, "Error: could not open logfile\n");
				exit(1);
			}
			break;
		case '?':
			if (strcmp(argv[optind-1], "--period") == 0) {
			 fprintf(stderr, "Error: --period missing argument\n");
			 exit(1);
			}
			else if (strcmp(argv[optind-1], "--scale") == 0) {
			 fprintf(stderr, "Error: --scale missing argument\n");
			 exit(1);
			}
			else if (strcmp(argv[optind-1], "--log") == 0) {
			 fprintf(stderr, "Error: --log missing argument\n");
			 exit(1);
			}
			else {
			 fprintf(stderr, "./lab4b: unrecognized option '%s'\n", argv[optind-1]);
			 fprintf(stderr, "Usage: ./la4b [--period=#] [--scale=] [C or F] [--log=filename]\n");
			 exit(1);
			}
			break;
		}
	}

	// Initialize Contexts
	init_contexts();

	// Only One Source of Input
	struct pollfd input[1];
	input[0].fd = STDIN_FILENO;
	input[0].events = POLLIN;

	// get first_time
	struct timeval first_time;
	gettimeofday(&first_time, NULL); // timezone don't care

	// Get First Temp, Print Out & Log First Report
	float first_temp = get_temp();
	struct tm* log_time;
	log_time = localtime(&(first_time.tv_sec));
	fprintf(stdout, "%02d:%02d:%02d %.1f\n", log_time->tm_hour, 
		log_time->tm_min, log_time->tm_sec, first_temp);
	if (log_flag) {
		int dprintf_ret = dprintf(4, "%02d:%02d:%02d %.1f\n", 
			log_time->tm_hour, log_time->tm_min, log_time->tm_sec, first_temp);
		// Error Checking dprintf()
		if (dprintf_ret < 0) {
			fprintf(stderr, "Error: could not write to logfile\n");
			exit(1);
		}
	}

	// Set Up Buffers
	char* buffer = (char*)malloc(128*sizeof(char));
	char* mini_buffer = (char*)malloc(128*sizeof(char));
	memset(buffer, 0, 128);
	memset(mini_buffer, 0, 128);

	// get_temp() based on --period, print or print & log
	while(1) {
		// get second_time
		struct timeval second_time;
		gettimeofday(&second_time, NULL);
		double difftime_ret = difftime(second_time.tv_sec, first_time.tv_sec);
		// if period has passed
		if (difftime_ret >= period_arg) {
			float temp_ret = get_temp();
			// print in spec format to stdout
			log_time = localtime(&(second_time.tv_sec));
			// "hour:min:sec temp", temp to 1 decimal point
			if (report_flag) {
				fprintf(stdout, "%02d:%02d:%02d %.1f\n", log_time->tm_hour, 
					log_time->tm_min, log_time->tm_sec, temp_ret);
			}
			// if we have a logfile, also log there
			if (log_flag) {
				if (report_flag) {
					int dprintf_ret = dprintf(4, "%02d:%02d:%02d %.1f\n", 
						log_time->tm_hour, log_time->tm_min, log_time->tm_sec, 
						temp_ret);
					// Error Checking dprintf()
					if (dprintf_ret < 0) {
						fprintf(stderr, "Error: could not write to logfile\n");
						exit(1);
					}
				}

			}
			// updating first_time for next comparison
			first_time.tv_sec = second_time.tv_sec;
		 	first_time.tv_usec = second_time.tv_usec;
		}
		int poll_ret = poll(input,1,500); // timeout 0.5 seconds
		// Error Checking poll()
		if (poll_ret < 0) {
			fprintf(stderr, "Error: unable to run poll()\n");
			exit(1);
		}
		// if theere is input from STDIN
		if(input[0].revents & POLLIN) {
			ssize_t bytes_read = read(STDIN_FILENO, buffer, 128);
			if (bytes_read < 0) {
    			fprintf(stderr, "Error: unable to run read()\n");
    			exit(1);
  			}
  			// processing buffer contents
  			int mini_i = 0;
  			for (int i = 0; i < bytes_read; i++) {
  				switch(buffer[i]) {
  					case '\n':
 				 	input_handling(mini_buffer);
 				 	memset(mini_buffer, 0, bytes_read);
 				 	mini_i = 0;
  					break;
  					default:
  					mini_buffer[mini_i] = buffer[i];
  					mini_i++;
  					break;
  				}
  			}
  			// clear buffer to read in again
  			memset(buffer, 0, bytes_read);
		}
		// button check every 0.5 seconds
		int button_ret = mraa_gpio_read(button);
		// if button is pressed, shutdown sequence
		if (button_ret == 1) {
			struct timeval off_time;
			gettimeofday(&off_time, NULL);
			struct tm* log_time;
			log_time = localtime(&(off_time.tv_sec));
			if (report_flag) {
				fprintf(stdout, "%02d:%02d:%02d SHUTDOWN\n", log_time->tm_hour, 
					log_time->tm_min, log_time->tm_sec);
			}
			if (log_flag) {
				if (report_flag) {
					dprintf(4, "OFF\n");
					dprintf(4, "%02d:%02d:%02d SHUTDOWN\n", log_time->tm_hour, 
						log_time->tm_min, log_time->tm_sec);
				}
			}
			exit(0);
		}

	}

	free(buffer);
	free(mini_buffer);

	// Close FD
	close(4);

	// Close IO Pins
	mraa_aio_close(temp_sensor);
	mraa_gpio_close(button);
	exit(0);  

}