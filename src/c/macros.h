/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================

  File: macros.h

  Macros for memory handling and debugging.
  =============================================================================
*/

/* -*- Mode: C -*- */

#ifndef _macros_h_
#define _macros_h_

/* This can be replaced by more efficient allocator */

#define NEWN(TYPE,NELEMS) ((TYPE*)malloc(sizeof(TYPE)*NELEMS))
#define NEW(TYPE) NEWN(TYPE,1)
#define DELETE(X) (free(X))

#undef DEBUG
/* #define DEBUG */

#ifdef DEBUG
#define DBG(...) fprintf(stderr,  __VA_ARGS__)
#define DBGCALL(OP) OP
#else
#define DBG(...)
#define DBGCALL(OP, ...)
#endif

#undef Assert
#ifndef NO_ASSERTS
#define Assert(x) if (!(x)) { fprintf(stderr, "Failed assertion " #x " at line %d of file %s.\n", \
				    __LINE__, __FILE__);		\
  exit(1);\
  }
#else
#define Assert(ignore)
#endif

#endif
