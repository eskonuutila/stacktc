/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: debug.h

  Printing the structs; used in debugging
  =============================================================================
*/
#ifndef _debug_h_
#define _debug_h_

#include "types.h"
#include "macros.h"
#include "util.h"

void print_vertex_struct(vint vertex_id, Vertex* vertex_table);

void print_scc_struct(vint scc_id, SCC** scc_table);

void print_vertex_stack(vint *vertex_stack, vint *vertex_stack_top);

#endif
