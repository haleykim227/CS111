// NAME: Seungwon Kim
// EMAIL: haleykim@g.ucla.edu
// ID: 405111152

#include <errno.h>
#include <sched.h>
#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definition of Variables
int opt_yield;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
  // if either list or element are invalid, return
  if (!(list || element)) {
    return;
  }
  // if list is not a valid list head, return
  else if (list->key != NULL) {
    return;
  }
  // if list only has list head
  else if ((list->next == list) && (list->prev == list)) {
    // yield before critical section
    if (opt_yield & INSERT_YIELD)
      sched_yield();
    // inserting node
    list->next = element;
    list->prev = element;
    element->next = list;
    element->prev = list;
  }
  // if list has list head and other element(s)
  else {
    char element_key[] = "aaaa"; // dummy value, overwritten soon
    strcpy(element_key, element->key);
    SortedListElement_t *curr = list->next;
    // start with curr on first element after list head
    // end when curr is back on list after going through every item
    while (curr != list) {
      // if we find a key larger than element's, insert element before larger key
      if (strcmp(curr->key, element_key) > 0) {
	// yield before critical section
	if (opt_yield & INSERT_YIELD)
	  sched_yield();
	// inserting node
	curr->prev->next = element;
	element->prev = curr->prev;
	element->next = curr;
	curr->prev = element;
	return;
      }
      curr = curr->next;
    }
    // if no key is larger than element's, append to end of list
    // yield before critical section
    if (opt_yield & INSERT_YIELD)
      sched_yield();
    // inserting node
    curr->prev->next = element;
    element->prev = curr->prev;
    element->next = list;
    curr->prev = element;
  }
}

int SortedList_delete( SortedListElement_t *element) {
  if (!element)
    return 1;
  if ((element->next->prev != element) || (element->prev->next != element))
    return 1;
  // yield before critical section
  if (opt_yield & DELETE_YIELD)
    sched_yield();
  // deleting node
  element->prev->next = element->next;
  element->next->prev = element->prev;
  return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  SortedListElement_t *curr = list->next;
  while (curr != list) {
    // if key of curr ptr is input key, return that node
    if (strcmp(curr->key, key) == 0) {
      // yield before returning found node
      if (opt_yield & LOOKUP_YIELD)
	sched_yield();
      return curr;
    }
    // advance curr to the next one
    curr = curr->next;
  }
  return NULL;
}

int SortedList_length(SortedList_t *list) {
  SortedListElement_t *curr = list->next;
  if (curr == list)
    return 0;
  int counter = 0;
  while (curr != list) {
    if ((curr->next->prev != curr) || (curr->prev->next != curr)) {
      // yield before returning -1
      if (opt_yield & LOOKUP_YIELD)
	sched_yield();
      return -1;
    }
    curr = curr->next;
    counter = counter + 1;
  }
  // yield before returning counter
  if (opt_yield & LOOKUP_YIELD)
    sched_yield();
  return counter;
}

/*
int main() {
  SortedListElement_t head;
  head.next = &head;
  head.prev = &head;
  head.key = NULL;
  
  SortedListElement_t temp;
  char temp_char[] = "aaaa";
  temp.key = temp_char;
  SortedList_insert(&head, &temp);
  
  SortedListElement_t second_temp;
  char second_temp_char[] = "bbbb";
  second_temp.key = second_temp_char;
  SortedList_insert(&head, &second_temp);

  SortedListElement_t third_temp;
  char third_temp_char[] = "aabb";
  third_temp.key = third_temp_char;
  SortedList_insert(&head, &third_temp);

  SortedList_lookup(&head, third_temp_char);

  char random_char[] = "zzzz";
  SortedList_lookup(&head, random_char);

  SortedList_length(&head);
}
*/
