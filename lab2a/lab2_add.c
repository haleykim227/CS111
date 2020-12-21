// NAME: Seungwon Kim
// EMAIL: haleykim@g.ucla.edu
// ID: 405111152

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Definition of Variables
int long_index = 0;
int threads_flag = 0;
int iterations_flag = 0;
int yield_flag = 0;
int sync_flag = 0;
int m_flag = 0;
int s_flag = 0;
int c_flag = 0;
int threads_arg = 0;
int iterations_arg = 0;

int lock = 0;
long long int counter = 0;
int opt_yield = 0;
char test_name[25] = "add-none"; // default test name
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Given add Function
void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
}

// add-m Function
void add_m(long long *pointer, long long value) {
  pthread_mutex_lock(&mutex);
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
  pthread_mutex_unlock(&mutex);
}

// add-s Function
void add_s(long long *pointer, long long value) {
  while(__sync_lock_test_and_set(&lock, 1));
  long long sum = *pointer + value;
  if (opt_yield)
      sched_yield();
  *pointer = sum;
  __sync_lock_release(&lock);
}

// add-c Function
void add_c(long long *pointer, long long value) {
  long long old_val, new_val;
  do {
    old_val = counter;
    new_val = old_val + value;
    if (opt_yield)
      sched_yield();
  } while (__sync_val_compare_and_swap(pointer, old_val, new_val) != old_val);
    // if pointer content is old_val, writes new_val into old_val
    // returns contents of pointer before operation
}

// Wrapping Function that Calls add Function
void* add_subtract(void *iterations_v) {
  int *iterations_i = (int*)iterations_v;
  for (int i = 0; i < *iterations_i; i++) {
    if (m_flag == 1) {
      add_m(&counter, 1);
      add_m(&counter, -1);
    }
    else if (s_flag == 1) {
      add_s(&counter, 1);
      add_s(&counter, -1);
    }
    else if (c_flag == 1) {
      add_c(&counter, 1);
      add_c(&counter, -1);
    }
    else {
      add(&counter, 1);
      add(&counter, -1);
    }
  }
  return NULL;
}

// Set Test Name
void set_test_name() {
  if (yield_flag == 0) {
    if (sync_flag && m_flag)
      strcpy(test_name, "add-m");
    else if (sync_flag && s_flag)
      strcpy(test_name, "add-s");
    else if (sync_flag && c_flag)
      strcpy(test_name, "add-c");
  }
  else {
    if (sync_flag == 0)
      strcpy(test_name, "add-yield-none");
    else if (sync_flag && m_flag)
      strcpy(test_name, "add-yield-m");
    else if (sync_flag && s_flag)
      strcpy(test_name, "add-yield-s");
    else if (sync_flag && c_flag)
      strcpy(test_name, "add-yield-c");
  }
}

// Main Routine
int main(int argc, char *argv[]) {

  while (1) {
    static struct option long_options[] = {{"threads", required_argument, 0, 't'},
                                           {"iterations", required_argument, 0, 'i'},
					   {"yield", no_argument, 0, 'y'},
					   {"sync", required_argument, 0, 's'},
                                           {0, 0, 0, 0}};
    int getopt_long_ret = getopt_long(argc, argv, "t:i:y", long_options, &long_index);

    // Break once parsing is complete                                                    
    if (getopt_long_ret == -1)
      break;

    // Setting variables and flags based on options and arguments 
    switch (getopt_long_ret) {
    case 't':
      threads_flag = 1;
      threads_arg = atoi(optarg);
      break;
    case 'i':
      iterations_flag = 1;
      iterations_arg = atoi(optarg);
      break;
    case 'y':
      opt_yield = 1;
      yield_flag = 1;
      break;
    case 's':
      sync_flag = 1;
      if (strcmp(optarg, "m") == 0)
	m_flag = 1;
      else if (strcmp(optarg, "s") == 0)
	s_flag = 1;
      else if (strcmp(optarg, "c") == 0)
	c_flag = 1;
      break;
    case '?':
      printf("./lab2_add: unrecognized option '%s'\n", argv[optind-1]);
      printf("Usage: ./lab2_add [--threads] [numThreads] [--iterations] [numIterations]\n");
      exit(1);
      break;
    }
  }

  // If the option was not called, default to 1
  if (threads_flag == 0)
    threads_arg = 1;
  if (iterations_flag == 0)
    iterations_arg = 1;

  // Setting Correct Test Name
  set_test_name();

  // Record Starting Time
  struct timespec time_start;
  int clock_gettime_ret = clock_gettime(CLOCK_MONOTONIC, &time_start);
  if (clock_gettime_ret != 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Error: Could not execute clock_gettime()\n");
    exit(2);
  }

  // Create Threads
  pthread_t threadId[threads_arg];
  for (int i = 0; i < threads_arg; i++) {
    int pthread_create_ret1 = pthread_create(&threadId[i], NULL,
					     &add_subtract, &iterations_arg);
    if (pthread_create_ret1 != 0) {
      fprintf(stderr, "Error: Could not create thread number: %d\n", i);
      exit(2);
    }
  }
  
  // Joining Threads
  for (int i = 0; i < threads_arg; i++) {
    int pthread_join_ret = pthread_join(threadId[i], NULL);
    if (pthread_join_ret != 0) {
      fprintf(stderr, "Error: Could not join thread number: %d\n", i);
      exit(2);
    }
  }

  // Record Ending Time
  struct timespec time_end;
  int clock_gettime_ret2 = clock_gettime(CLOCK_MONOTONIC, &time_end);
  if (clock_gettime_ret2 != 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Error: Could not execute clock_gettime()\n");
    exit(2);
  }

  // Calculate Number of Operations
  int num_operations = threads_arg * iterations_arg * 2;
  
  // Calculate Total Runtime
  double sec_diff_t = difftime(time_end.tv_sec, time_start.tv_sec);
  unsigned long sec_to_nanosec = sec_diff_t * 1000000000;
  unsigned long nanosec = time_end.tv_nsec - time_start.tv_nsec;
  unsigned long runtime = sec_to_nanosec + nanosec;
  
  // Calculate Averate Time Per Operation
  unsigned long avg_time = runtime / num_operations;
  
  // Screen Output
  printf("%s,%d,%d,%d,%lu,%lu,%llu\n", test_name, threads_arg, iterations_arg,
	 num_operations, runtime, avg_time, counter);
}
