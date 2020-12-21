// NAME: Seungwon Kim
// EMAIL: haleykim@g.ucla.edu
// ID: 405111152

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include "SortedList.h"
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
int threads_arg = 0;
int iterations_arg = 0;
int lock = 0;
int opt_yield = 0;
char yield_arg[25];
char sync_arg[25];
char none[] = "-none";
char m_str[] = "-m";
char s_str[] = "-s";
char test_name[25] = "list-"; // default test name
long long int counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
time_t t;
SortedListElement_t* elements;
SortedList_t *head;

/* Helper Functions */

// Set Test Name
void set_test_name() {
  if (yield_flag == 0) {
    if (sync_flag && m_flag)
      strcpy(test_name, "list-none-m");
    else if (sync_flag && s_flag)
      strcpy(test_name, "list-none-s");
    else if (!sync_flag)
      strcpy(test_name, "list-none-none");
  }
  else {
    if (sync_flag == 0) {
      strcat(test_name, yield_arg);
      strcat(test_name, none);
    }
    else if (sync_flag && m_flag) {
      strcat(test_name, yield_arg);
      strcat(test_name, m_str);
    }
    else if (sync_flag && s_flag) {
      strcat(test_name, yield_arg);
      strcat(test_name, s_str);
    }
  }
}

// Set opt_yield
void set_opt_yield() {
  for (size_t i = 0; i < strlen(yield_arg); i++) {
    if (yield_arg[i] == 'i')
      opt_yield = opt_yield | INSERT_YIELD;
    else if (yield_arg[i] == 'd')
      opt_yield = opt_yield | DELETE_YIELD;
    else if (yield_arg[i] == 'l')
      opt_yield = opt_yield | LOOKUP_YIELD;
  }
}

// Create Random Key
char* random_key() {
  char* random_key = (char*)malloc(sizeof(char)*5);
  random_key[0] = rand() % 26 + 'a';
  random_key[1]	= rand() % 26 + 'a';
  random_key[2]	= rand() % 26 + 'a';
  random_key[3]	= rand() % 26 + 'a';
  random_key[4] = '\0';
  return random_key;
}

// Handle Segmentation Faults
void signal_handler(int signum) {
  if (signum == SIGSEGV) {
      fprintf(stderr, "Segmentation fault: caught and handled\n");
      exit(2);
    }
}

// Function Called by pthread_create()
void* thread_work(void* current_input) {
  int elements_i = *((int*)current_input);
  // if we're using mutex, lock
  if (m_flag)
    pthread_mutex_lock(&mutex);
  // if we're using spin lock, lock
  else if (s_flag)
    while (__sync_lock_test_and_set(&lock, 1));
  // the actual task for thread
  // insert elements
  for (int i = elements_i; i < elements_i + iterations_arg; i++) {
    SortedList_insert(head, &elements[i]);
  }
  int length = SortedList_length(head);
  if (length < 0) {
    fprintf(stderr, "Error: length is negative after insertion\n");
    exit(2);
  }
  // lookup and delete elements
  SortedListElement_t* inserted;
  for (int i = elements_i; i < elements_i + iterations_arg; i++) {
    inserted = SortedList_lookup(head, elements[i].key);
    // Check for Inconsistencies
    if (inserted == NULL) {
      fprintf(stderr, "Error: could not lookup element due to inconsistencies\n");
      exit(2);
    }
    int delete_ret = SortedList_delete(inserted);
    // Check for Inconsistencies
    if (delete_ret == 1) {
      fprintf(stderr, "Error: could not delete element due to inconsistencies\n");
      exit(2);
    }
  }
  // unlock mutex
  if (m_flag)
    pthread_mutex_unlock(&mutex);
  // unlock spinlock
  else if (s_flag)
    __sync_lock_release(&lock);
  return NULL;
}

// Main Routine
int main(int argc, char *argv[]) {
  signal(SIGSEGV, signal_handler);
  while (1) {
    static struct option long_options[] = {{"threads", required_argument, 0, 't'},
                                           {"iterations", required_argument, 0, 'i'},
                                           {"yield", required_argument, 0, 'y'},
					   {"sync", required_argument, 0, 's'},
                                           {0, 0, 0, 0}};
    int getopt_long_ret = getopt_long(argc, argv, "t:i:y:s:", long_options, &long_index);

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
      strcpy(yield_arg, optarg);
      break;
    case 's':
      sync_flag = 1;
      strcpy(sync_arg, optarg);
      if (strcmp(optarg, "m") == 0)
	m_flag = 1;
      if (strcmp(optarg, "s") == 0)
	s_flag = 1;
      break;
    case '?':
      printf("./lab2_list: unrecognized option '%s'\n", argv[optind-1]);
      printf("Usage: ./lab2_list [--threads] [numThreads] [--iterations] [numIterations]\n");
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

  // Setting opt_yield
  set_opt_yield();

  // Create Empty List
  head = (SortedList_t*)malloc(sizeof(SortedList_t));
  // Error Checking malloc()
  if (head == NULL) {
    fprintf(stderr, "Error: could not allocate memory for head\n");
    exit(1);
  }
  head->next = head;
  head->prev = head;
  head->key = NULL;
  
  // Create Required Number of Elements
  int num_elements = threads_arg * iterations_arg;
  elements = (SortedListElement_t*)malloc(sizeof(SortedListElement_t)*num_elements);
  // Error Checking malloc()
  if (elements == NULL) {
    fprintf(stderr, "Error: could not allocate memory for elements\n");
    exit(1);
  }
  // Creating Random Number Generator
  srand((unsigned int) time(&t));
  // Creating Keys for Each Element
  for (int i = 0; i < num_elements; i++) {
    elements[i].key = random_key();
  }
  
  // Record Starting Time
  struct timespec time_start;
  int clock_gettime_ret = clock_gettime(CLOCK_MONOTONIC, &time_start);
  if (clock_gettime_ret != 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Error: Could not execute clock_gettime()\n");
    exit(1);
  }

  // Create Threads
  pthread_t threadId[threads_arg];
  int current_input[threads_arg];
  for (int i = 0; i < threads_arg; i++) {
    current_input[i] = i * iterations_arg;
    int pthread_create_ret1 = pthread_create(&threadId[i], NULL,
                                             &thread_work, &current_input[i]);
    // Error Checking pthread_create()
    if (pthread_create_ret1 != 0) {
      fprintf(stderr, "Error: Could not create thread number: %d\n", i);
      exit(1);
    }
  }
  
  // Joining Threads
  for (int i = 0; i < threads_arg; i++) {
    int pthread_join_ret = pthread_join(threadId[i], NULL);
    if (pthread_join_ret != 0) {
      fprintf(stderr, "Error: Could not join thread number: %d\n", i);
      exit(1);
    }
  }

  // Record Ending Time
  struct timespec time_end;
  int clock_gettime_ret2 = clock_gettime(CLOCK_MONOTONIC, &time_end);
  if (clock_gettime_ret2 != 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Error: Could not execute clock_gettime()\n");
    exit(1);
  }

  // Check Whether List is Corrupted
  if (SortedList_length(head) != 0){
    fprintf(stderr, "Error: list is corrupted\n");
    exit(2);
  }

  // Calculate Number of Operations
  int num_operations = threads_arg * iterations_arg * 3;

  // Calculate Total Runtime
  double sec_diff_t = difftime(time_end.tv_sec, time_start.tv_sec);
  unsigned long sec_to_nanosec = sec_diff_t * 1000000000;
  unsigned long nanosec = time_end.tv_nsec - time_start.tv_nsec;
  unsigned long runtime = sec_to_nanosec + nanosec;

  // Calculate Averate Time Per Operation
  unsigned long avg_time = runtime / num_operations;

  // Free Memory
  free(elements);
  
  // Screen Output
  printf("%s,%d,%d,1,%d,%lu,%lu\n", test_name, threads_arg, iterations_arg,
         num_operations, runtime, avg_time);

  exit(0);
}
