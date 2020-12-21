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
int list_flag = 0;
int threads_arg = 0;
int iterations_arg = 0;
int list_arg = 0;
int opt_yield = 0;
int* locks;
int* hash_nums;
char yield_arg[25];
char sync_arg[25];
char none[] = "-none";
char m_str[] = "-m";
char s_str[] = "-s";
char test_name[25] = "list-"; // default test name
long total_wait = 0;
long long int counter = 0;
pthread_mutex_t* mutex;
time_t t;
SortedListElement_t* elements;
SortedList_t* heads;

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

// Hash Function to Choose Sublist
int hash_func(const char* key) {
  int sum = 0;
  for (int i = 0; i < 4; i++) {
    sum = sum + key[i];
  }
  return sum % list_arg;
}

// Function Called by pthread_create()
void* thread_work(void* current_input) {
  // Cast and De-Reference Input
  int elements_i = *((int*)current_input);
  // Time Spent Waiting for Locks: This Thread Only
  long thread_wait = 0;
  struct timespec begin;
  struct timespec end;
  // Insertion
  for (int i = elements_i; i < elements_i + iterations_arg; i++) {
    // Mutex
    if (m_flag) {
      // record time, get lock, record time
      clock_gettime(CLOCK_MONOTONIC, &begin);
      pthread_mutex_lock(&mutex[hash_nums[i]]);
      clock_gettime(CLOCK_MONOTONIC, &end);
      thread_wait += 1000*1000*1000 * (end.tv_sec - begin.tv_sec) +
	(end.tv_nsec - begin.tv_nsec);
      SortedList_insert(&heads[hash_nums[i]], &elements[i]);
      pthread_mutex_unlock(&mutex[hash_nums[i]]);
    }
    // Spin-Lock
    else if (s_flag) {
      // record time, get lock, record time
      clock_gettime(CLOCK_MONOTONIC, &begin);
      while(__sync_lock_test_and_set(&locks[hash_nums[i]], 1));
      clock_gettime(CLOCK_MONOTONIC, &end);
      thread_wait += 1000*1000*1000 * (end.tv_sec - begin.tv_sec) +
	(end.tv_nsec - begin.tv_nsec);
      SortedList_insert(&heads[hash_nums[i]], &elements[i]);
      __sync_lock_release(&locks[hash_nums[i]]);
    }
    else {
      SortedList_insert(&heads[hash_nums[i]], &elements[i]);
    }
  }
  // Length
  int total_length = 0;
  for (int i = 0; i < list_arg; i++) {
    if (m_flag) {
      clock_gettime(CLOCK_MONOTONIC, &begin);
      pthread_mutex_lock(&mutex[i]);
      clock_gettime(CLOCK_MONOTONIC, &end);
      thread_wait += 1000*1000*1000 * (end.tv_sec - begin.tv_sec) +
	(end.tv_nsec - begin.tv_nsec);
      int sub_length = SortedList_length(&heads[i]);
      total_length = total_length + sub_length;
      pthread_mutex_unlock(&mutex[i]);
    }
    else if (s_flag) {
      clock_gettime(CLOCK_MONOTONIC, &begin);
      while(__sync_lock_test_and_set(&locks[i], 1));
      clock_gettime(CLOCK_MONOTONIC, &end);
      thread_wait += 1000*1000*1000 * (end.tv_sec - begin.tv_sec) +
	(end.tv_nsec - begin.tv_nsec);      
      int sub_length = SortedList_length(&heads[i]);
      total_length = total_length + sub_length;
      __sync_lock_release(&locks[i]);
    }
    else {
      int sub_length = SortedList_length(&heads[i]);
      total_length = total_length + sub_length;
    }
  }
  // Lookup and Delete Elements
  SortedListElement_t* inserted;
  for (int i = elements_i; i < elements_i + iterations_arg; i++) {
    if (m_flag) {
      clock_gettime(CLOCK_MONOTONIC, &begin);
      pthread_mutex_lock(&mutex[hash_nums[i]]);
      clock_gettime(CLOCK_MONOTONIC, &end);
      thread_wait += 1000*1000*1000 * (end.tv_sec - begin.tv_sec) +
	(end.tv_nsec - begin.tv_nsec);
      inserted = SortedList_lookup(&heads[hash_nums[i]], elements[i].key);
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
      pthread_mutex_unlock(&mutex[hash_nums[i]]);
    }
    else if (s_flag) {
      clock_gettime(CLOCK_MONOTONIC, &begin);
      while(__sync_lock_test_and_set(&locks[hash_nums[i]], 1));
      clock_gettime(CLOCK_MONOTONIC, &end);
      thread_wait += 1000*1000*1000 * (end.tv_sec - begin.tv_sec) +
	(end.tv_nsec - begin.tv_nsec);
      inserted = SortedList_lookup(&heads[hash_nums[i]], elements[i].key);
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
      __sync_lock_release(&locks[hash_nums[i]]);
    }
    else {
      inserted = SortedList_lookup(&heads[hash_nums[i]], elements[i].key);
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
  }
  return (void*) thread_wait;
}

// Main Routine
int main(int argc, char *argv[]) {
  signal(SIGSEGV, signal_handler);
  while (1) {
    static struct option long_options[] = {{"threads", required_argument, 0, 't'},
                                           {"iterations", required_argument, 0, 'i'},
                                           {"yield", required_argument, 0, 'y'},
					   {"sync", required_argument, 0, 's'},
					   {"lists", required_argument, 0, 'l'},
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
    case 'l':
      list_flag = 1;
      list_arg = atoi(optarg);
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
  if (list_flag == 0)
    list_arg = 1;

  // Setting Correct Test Name
  set_test_name();

  // Setting opt_yield
  set_opt_yield();

  // Creating Empty Lists
  heads = (SortedList_t*)malloc(sizeof(SortedList_t) * list_arg);
  // Error Checking malloc()
  if (heads == NULL) {
    fprintf(stderr, "Error: could not allocate memory for head\n");
    exit(1);
  }
  for (int i = 0; i < list_arg; i++) {
    heads[i].next = &heads[i];
    heads[i].prev = &heads[i];
    heads[i].key = NULL;
  }
  
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

  // Initializing mutexes
  mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * list_arg);
  // Error Checking malloc()
  if (mutex == NULL) {
    fprintf(stderr, "Error: could not allocate memory for mutexes\n");
    exit(1);
  }
  // Initializing Each Mutex
  for (int i = 0; i < list_arg; i++) {
    int mutex_init_ret = pthread_mutex_init((mutex + i), NULL);
    // Error Checking pthread_mutex_init()
    if (mutex_init_ret != 0) {
      fprintf(stderr, "Error: could not initialize mutexes\n");
      exit(1);
    }
  }

  // Initializing Spin-Locks
  locks = (int*)malloc(sizeof(int) * list_arg);
  // Error Checking malloc()
  if (locks == NULL) {
    fprintf(stderr, "Error: could not allocate memory for spin-locks\n");
    exit(1);
  }
  // Initializing Each Spin-Lock
  for (int i = 0; i < list_arg; i++) {
    locks[i] = 0;
  }
  
  // Assigning Hash Number to Each Element
  hash_nums = (int*)malloc(sizeof(int)* num_elements);
  for (int i = 0; i < num_elements; i++) {
    hash_nums[i] = hash_func(elements[i].key);
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
  void** wait_ret = (void**)malloc(sizeof(void**));
  for (int i = 0; i < threads_arg; i++) {
    int pthread_join_ret = pthread_join(threadId[i], wait_ret);
    if (pthread_join_ret != 0) {
      fprintf(stderr, "Error: Could not join thread number: %d\n", i);
      exit(1);
    }
    total_wait += (long) *wait_ret;
  }

  // Record Ending Time
  struct timespec time_end;
  int clock_gettime_ret2 = clock_gettime(CLOCK_MONOTONIC, &time_end);
  if (clock_gettime_ret2 != 0) {
    fprintf(stderr, "Error Number: %d, Error Message: %s\n", errno, strerror(errno));
    fprintf(stderr, "Error: Could not execute clock_gettime()\n");
    exit(1);
  }

  // Checking Length of Entire List
  long list_length = 0;
  for (int i = 0; i < list_arg; i++) {
    list_length += SortedList_length(heads + i);
  }
  if (list_length != 0) {
    fprintf(stderr, "Error: final length is not 0\n");
    exit(2);
  }

  // Calculate Number of Operations
  int num_operations = threads_arg * iterations_arg * 3;

  // Calculate Average Wait-For-Lock Timee
  long avg_wait = total_wait / num_operations; 

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
  printf("%s,%d,%d,%d,%d,%lu,%lu,%ld\n", test_name, threads_arg, iterations_arg, list_arg,
	 num_operations, runtime, avg_time, avg_wait);

  exit(0);
}
