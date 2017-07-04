// -*- Mode: C++ -*-
//
// $RCSfile$
// $Revision$
// $Source$
// $Author$
// $Date$
// $State$
// $Log$
//
// Description:
//
// This file implements several simple utility functions.
//
// Ifdefs:
//  
// DEBUG_ALLOCATIONS: if non-zero, collects information about memory allocations.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "allocs.h"

int *_new_int_table(int size, int initial_element)
{
#ifdef DEBUG_ALLOCATIONS
  int saved_lno;
  char *saved_fle;
  char *saved_cmd;
  if (debug_level == DebugNoisy) {
    saved_lno = _lno_;
    saved_fle = _fle_;
    saved_cmd = _cmd_;
  }
#endif
  int *result = (new int[size]);
  for (int i = 0; i < size; i++)
    result[i] = initial_element;
#ifdef DEBUG_ALLOCATIONS
  if (debug_level == DebugNoisy)
    fprintf(stderr,
	    "new_int_table(%d, %d) [%ld bytes] (%s) at line %ld of file %s -> %lx\n", 
	    size, initial_element, (long)(sizeof(int)*size), 
	    saved_cmd, (long)saved_lno, saved_fle, (long)result);
#endif
  return result;
}

char *_copy_string(const char* string) {
#ifdef DEBUG_ALLOCATIONS
  int saved_lno;
  char *saved_fle;
  char *saved_cmd;
  if (debug_level == DebugNoisy) {
    saved_lno = _lno_;
    saved_fle = _fle_;
    saved_cmd = _cmd_;
  }
#endif
  if (!string) return 0;
  char *new_string = (new char[strlen(string)+1]);
  if (!new_string) {
    printf_abort("Cannot create a copy of string %s!\n", string);
    return 0;
  } else {
    strcpy(new_string, string);
#ifdef DEBUG_ALLOCATIONS
    if (debug_level == DebugNoisy)
      fprintf(stderr, 
	      "copy_string(%s) [%ld bytes] (%s) at line %ld of file %s -> %lx(%s)\n", 
	      string, (long)(strlen(string)+1), saved_cmd, (long)saved_lno, saved_fle, 
	      (long)new_string, new_string);
#endif
    return new_string;
  }
}

char *_concat_string(const char* a, const char* b) {
#ifdef DEBUG_ALLOCATIONS
  int saved_lno;
  char *saved_fle;
  char *saved_cmd;
  if (debug_level == DebugNoisy) {
    saved_lno = _lno_;
    saved_fle = _fle_;
    saved_cmd = _cmd_;
  }
#endif
  char *new_string = (new char[strlen(a)+strlen(b)+1]);
  if (!new_string) {
    printf_abort("Cannot create a concatenate strings %s and %s!\n", a, b);
    return 0;
  } else {
    strcpy(new_string, a);
    strcat(new_string, b);
#ifdef DEBUG_ALLOCATIONS
    if (debug_level == DebugNoisy)
      fprintf(stderr, 
	      "concat_string(%s, %s) [%ld bytes] (%s) at line %ld of file %s -> %lx(%s)\n", 
	      a, b, (long)(strlen(new_string)+1), saved_cmd, (long)saved_lno, saved_fle, 
	      (long)new_string, new_string);
#endif
    return new_string;
  }
}

char alloc_sprintf_buffer[10000];

char* alloc_sprintf(const char* format ...) {
  va_list ap;
  va_start(ap, format);
  vsprintf(alloc_sprintf_buffer, format, ap);
  va_end(ap);
  return copy_string(alloc_sprintf_buffer);
}

void check_table(int *table, int n_elements, int l_bound, int h_bound)
{
  for (int i = 0; i < n_elements; i++) {
    int elem = table[i];
    if (elem < l_bound || elem > h_bound) abort();
  }
}

int volatile _vfprintf_error(ErrorOperation oper, FILE* stream, const char *format, va_list ap) {
  vfprintf(stream, format, ap);
  switch (oper) {
  case ErrorExit:
    exit(1);
  case ErrorAbort:
    abort();
  case ErrorContinue:
    break;
  default:
    fprintf(stderr, "Error in error operation!\n");
    abort();
  }
  return 0;
}

int volatile _fprintf_error(ErrorOperation oper, FILE* stream, const char *format ...) {
  va_list ap;
  va_start(ap, format);
  _vfprintf_error(oper, stream, format, ap);
  va_end(ap);
  return 0;
}

int volatile _printf_exit(const char* format ...) {
  va_list ap;
  va_start(ap, format);
  _vfprintf_error(ErrorExit, stderr, format, ap);
  va_end(ap);
  return 0;
}

int volatile _printf_abort(const char* format ...) {
  va_list ap;
  va_start(ap, format);
  _vfprintf_error(ErrorAbort, stderr, format, ap);
  va_end(ap);
  return 0;
}

double usec_sub(struct timeval *tp_later, struct timeval *tp_earlier) {
  return 1000000.0*(tp_later->tv_sec - tp_earlier->tv_sec)
    + tp_later->tv_usec - tp_earlier->tv_usec;
}

int cmp_int(const void *a, const void *b) {
  return (*((int*)a) - *((int*)b));
}

