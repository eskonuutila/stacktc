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
// This file declares several simple utility functions and macros.
//

#ifndef _util_h
#define _util_h

/* #include <builtin.h> */
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#if defined(_AIX)
#define irint(x) ((int)(rint(x)))
#define DefineTimezone struct timezone tzone 
#define GetTimeOfDay(x) gettimeofday(x,&tzone)
extern "C" {
  int gettimeofday(struct timeval*, struct timezone*);
}
#elif defined(__svr4__)
#define irint(x) ((int)(rint(x)))
extern "C" {
  int gettimeofday(struct timeval *tp, void *);
}
#define DefineTimezone 
#define GetTimeOfDay(x) gettimeofday(x, 0)
#elif defined(__linux__)
#define irint(x) ((int)(rint(x)))
#define DefineTimezone struct timezone tzone 
#define GetTimeOfDay(x) gettimeofday(x,&tzone)
extern "C" {
  int gettimeofday(struct timeval*, struct timezone*);
}
#else
#define DefineTimezone struct timezone tzone 
#define GetTimeOfDay(x) gettimeofday(x,&tzone)
extern "C" {
  int gettimeofday(struct timeval*, struct timezone*);
}
#endif

/* General purpose macros */

#define CONC(a,b) a ## b
#define NAME1(cl,arg) CONC(cl,arg)
#define FROB(name) #name
#define TO_STRING(name) FROB(name)

int *_new_int_table(int size, int initial_element);

void check_table(int *table, int n_elements, int l_bound, int h_bound);

char *_copy_string(const char* string);
char *_concat_string(const char *a, const char *b);
char *alloc_sprintf(const char *format ...);

#define copy_string(str) HERE(_copy_string(str))
#define concat_string(a, b) HERE(_concat_string(a, b))

enum ErrorOperation { ErrorExit, ErrorAbort, ErrorContinue };

int volatile _vfprintf_error(ErrorOperation oper, FILE* stream, const char *format, va_list ap);

int volatile _fprintf_error(ErrorOperation oper, FILE *stream, const char *format ...);

int volatile _printf_exit(const char* format ...);

int volatile _printf_abort(const char* format ...);

#ifndef fprintf_error
#define fprintf_error _fprintf_error
#endif

#ifndef printf_exit
#define printf_exit _printf_exit
#endif

#ifndef printf_abort
#define printf_abort _printf_abort
#endif

double usec_sub(struct timeval *tp_later, struct timeval *tp_earlier);

int cmp_int(const void *a, const void *b);

#define insert_to_table(type,element,table,table_size,element_count)\
do {							\
  if (element_count == table_size) {			\
    table_size *= 2;					\
    type* dummy_new_##table = NEW(type[table_size]);	\
    for (int dummy_##table##_index = 0;			\
	 dummy_##table##_index < element_count;		\
	 dummy_##table##_index ++)			\
      dummy_new_##table[dummy_##table##_index] = 	\
	table[dummy_##table##_index];			\
    DELETE_ARRAY(table);				\
    table = dummy_new_##table;				\
  }							\
  table[element_count++] = element;			\
} while (0)

#undef Assert
#ifndef NO_ASSERTS
#define Assert(x) ((x) ? 1 : (fprintf_error(ErrorAbort, stderr, \
			    "Failed assertion " #x " at line %d of file %s.\n",\
					    __LINE__, __FILE__), 0))
#else
#define Assert(ignore)
#endif

#ifdef __GNUC__
#define FUNC_UNUSED __attribute__ ((unused))
#define FUNC_NORETURN __attribute__ ((noreturn))
#else
#define FUNC_UNUSED
#define FUNC_NORETURN
#endif

#endif
