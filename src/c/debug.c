/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: debug.c

  Printing the structs; used in debugging
  =============================================================================
*/

#include "debug.h"

void print_vertex_struct(vint vertex_id, Vertex* vertex_table) {
  Vertex *vertex = vertex_table + vertex_id;
  vint outdegree = vertex->outdegree;
  vint *children = vertex->children;
  fprintf(stderr, "vertex[" VFMT "] " VFMT " children: ", vertex_id, outdegree);
  for (vint i = 0; i < outdegree; i++) {
    fprintf(stderr, " " VFMT, children[i]);
  }
  fprintf(stderr, "\n");
}


void print_scc_struct(vint scc_id, SCC** scc_table) {
  SCC *scc = scc_table[scc_id];
  vint root_vertex_id = scc->root_vertex_id;
  vint vertex_count = scc->vertex_count;
  vint *vertices = scc->vertex_table;
  fprintf(stderr, "scc[" VFMT "]: root vertex = " VFMT " contains " VFMT " vertices", scc_id, root_vertex_id, vertex_count);
  for (vint i = 0; i < vertex_count; i++) {
    fprintf(stderr, " " VFMT, vertices[i]);
  }
  fprintf(stderr, "\n");
}


void print_vertex_stack(vint *vertex_stack, vint *vertex_stack_top) {
  vint n = vertex_stack_top - vertex_stack;
  for (vint i = 0; i < n; i++) {
    fprintf(stderr, "    vertex_stack[" VFMT "] = " VFMT "\n", i, vertex_stack[i]);
  }
}
