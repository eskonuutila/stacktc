/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: types.h

  Datatypes used in the program.
  =============================================================================
*/

#ifndef _types_h_
#define _types_h_

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

typedef struct edge_struct {
  vint from;
  vint to;
} EDGE;

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

typedef struct matrix_struct {
  vint n;
  vint *elements;
} Matrix;

enum output_format {
  output_vertices = 1,
  output_edges = 2,
  output_components = 3,
  output_component_edges = 4,
  output_intervals = 5,
  output_nothing = 6
};

#endif
