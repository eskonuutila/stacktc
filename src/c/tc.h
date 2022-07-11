/* -*- Mode: C -*- */

#ifndef _tc_h_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define vint long
#define VFMT "%ld"

typedef struct vertex_struct {
  vint vertex_id;
  vint outdegree;
  vint *children;
} Vertex;

typedef struct interval_struct {
  vint low, high;
} Interval;

typedef struct intervals_struct {
  Interval* interval_table;
  vint interval_count;
} Intervals;

typedef struct scc_struct {
  vint scc_id;
  vint root_vertex_id;
  vint *vertex_table;
  vint vertex_count;
  Intervals* successors;
} SCC;

typedef struct digraph_struct {
  Vertex *vertex_table;
  vint vertex_count;
  vint *edge_table;
  vint edge_count;
} Digraph;

typedef struct tc_struct {
  SCC **scc_table;
  vint scc_count;
  vint *vertex_table; /* All vertices of all components are in the same table */
  vint vertex_count; /* Shows the position where new vertex is put */
  vint *vertex_id_to_scc_id_table;
  vint saved_vertex_count; /* Used for counting the number of vertices in a component */
} TC;

typedef struct tc_scc_iter_struct {
  int reversep;
  TC *tc;
  Intervals *intervals;
  vint current_interval_index;
  vint interval_limit;
  vint to_scc_id;
  vint to_scc_limit;
} TCSCCIter;

const int ITER_FINISHED = -12345;

typedef struct matrix_struct {
  vint n;
  vint *elements;
} Matrix;

/* This can be replaced by more efficient allocator */

#define NEWN(TYPE,NELEMS) ((TYPE*)malloc(sizeof(TYPE)*NELEMS))
#define NEW(TYPE) NEWN(TYPE,1)
#define DELETE(X) (free(X))

#undef WITH_MSGS
#ifdef WITH_MSGS
#define SHOW_MSGS (1)
#define MSG(...) fprintf(stderr,  __VA_ARGS__)
#else
#define MSG(...)
#define SHOW_MSGS (0)
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
