/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: algorithm.c

  The algorithm stacktc described in section 3.4 of
  E. Nuutila: Efficient transitive closure computation in large digraphs,
  PhD thesis, Helsinki University of Technology, Laboratory of Information
  Processing Science, 1995.

  https://github.com/eskonuutila/tc/blob/master/docs/thesis.pdf

  =============================================================================
*/

#include "algorithm.h"

/*==== Global variables ==== */

vint *vertex_stack, *vertex_stack_top;
vint *depth_first_numbers;
vint depth_first_number_counter;
Vertex *vertex_table;
TC *tc;
vint *vertex_id_to_scc_id_table;
SCC **scc_table;
vint *scc_stack, *scc_stack_top;

/* If the vertex has already been visited, do nothing. Otherwise recursively
   detect the strong component containing the vertex and compute its transitive
   closure. */
static vint visit(vint vertex_id) {
  vint dfn, lowest;
  vint self_loop_p = 0;
  vint *scc_stack_position = scc_stack_top;
  vint edge;
  Vertex *vertex = vertex_table + vertex_id;
  DBG("ENTER visit(" VFMT "), depth_first_numbers[" VFMT "] = " VFMT "\n", vertex_id, vertex_id, depth_first_numbers[vertex_id]);
  if (depth_first_numbers[vertex_id] >= 0) {
    DBG("Already visited, ignore\n");
  } else {
    DBG("Processing ");
    DBGCALL(print_vertex_struct(vertex_id, vertex_table));
    DBG("PUSH VERTEX " VFMT " to vertex_stack[" VFMT "]\n", vertex_id, vertex_stack_top - vertex_stack);
    *(vertex_stack_top++) = vertex_id;
    DBGCALL(print_vertex_stack(vertex_stack, vertex_stack_top));
    depth_first_numbers[vertex_id] = dfn = lowest = depth_first_number_counter++;
    DBG("Set depth_first_numbers[" VFMT "] = " VFMT "\n", vertex_id, dfn);
    for (edge = 0; edge != vertex->outdegree; edge++) {
      vint child = vertex->children[edge];
      vint child_value = depth_first_numbers[child];
      DBG("Processing child[" VFMT "]=" VFMT ", dfn[" VFMT "] = " VFMT ", lowest[" VFMT "] = " VFMT ", child_value = " VFMT "\n",
	      vertex_id, edge, vertex_id, dfn, vertex_id, lowest, depth_first_numbers[child]);
      if (child_value < 0) {
	DBG("Tree edge (" VFMT ", " VFMT "), visit(" VFMT ")\n", vertex_id, child, child);
	child_value = visit(child);
	if (child_value < lowest) {
	  DBG("Visit (" VFMT ") returned new lowest " VFMT "\n", child, child_value);
	  lowest = child_value;
	} else {
	  DBG("Visit (" VFMT ") returned " VFMT "\n", child, child_value);
	}
      } else if (child_value > dfn) {
	DBG("Forward edge (" VFMT ", " VFMT "), ignore\n", vertex_id, child);
      } else {
	vint child_scc_id = vertex_id_to_scc_id_table[child];
	if (child_scc_id >= 0) {
	  DBG("Intercomponent cross edge (" VFMT "," VFMT ")\npush " VFMT " to scc_stack[" VFMT "]\n", vertex_id, child, child_scc_id, scc_stack_top-scc_stack);
	  *(scc_stack_top++) = child_scc_id;
	} else if (child_value < lowest) {
	  DBG("Back edge or intracomponent cross edge (" VFMT "," VFMT ")\nlowest = " VFMT "\n", vertex_id, child, child_value);
	  lowest = child_value;
	} else if (child == vertex_id) {
	  DBG("Self loop edge (" VFMT "," VFMT ")\n", vertex_id, vertex_id);
	  self_loop_p = 1;
	}
      }
    }
    DBG("All children of " VFMT " processed, lowest = " VFMT ", dfn = " VFMT "\n", vertex_id, lowest, dfn);
    if (lowest == dfn) {
      DBG("Vertex " VFMT " is the component root\n", vertex_id);
      SCC *new_scc = TC_create_scc(tc, vertex_id);
      vint scc_id = new_scc->scc_id;
      DBG("generate new component " VFMT ", root = " VFMT "\n", scc_id, vertex_id);
      vint self_insert = self_loop_p || (*(vertex_stack_top-1) != vertex_id);
      DBG("self_insert = " VFMT ", self_loop = " VFMT "\n", self_insert, self_loop_p);
      Intervals *succ = 0;
      vint component_count = scc_stack_top - scc_stack_position;
      DBG("scc_stack contains " VFMT " adjacent components of " VFMT "\n", component_count, scc_id);
      if (self_insert || component_count) {
	DBG("Creating successor set for component " VFMT "\n", scc_id);
	succ = new_scc->successors = Intervals_new();
      }
      if (component_count) {
	vint prev_scc_id = -1;
	DBG("Sort adjacent components\n");
	qsort(scc_stack_position, component_count, sizeof(vint), &cmp_vint);
	DBG("Scanning adjacent components of " VFMT " on scc_stack\n", scc_id);
	while (scc_stack_top != scc_stack_position) {
	  vint scc_id = *(--scc_stack_top);
	  DBG("Popping adjacent component " VFMT " from scc_stack[" VFMT "]\n", scc_id, scc_stack_top-scc_stack);
	  if (scc_id != prev_scc_id) {
	    if (!(Intervals_insert(succ, scc_id))) {
	      DBG("Component " VFMT " not in Succ[" VFMT "], unioning with Succ[" VFMT "]\n", scc_id, scc_id, scc_id);
	      Intervals_union(succ, scc_table[scc_id]->successors);
	    } else {
	      DBG("Component " VFMT " already in Succ[" VFMT "]\n", scc_id, scc_id);
	    }
	    prev_scc_id = scc_id;
	  } else {
	    DBG("Ignoring duplicate " VFMT " in scc_stack[" VFMT "]\n", scc_id, scc_stack_top-scc_stack);
	  }
	}
	DBG("All adjacent components of " VFMT " processed\n", scc_id);
      }
      if (self_insert) {
	Intervals_insert(succ, scc_id);
	DBG("Inserting " VFMT " to its own successor set\n", scc_id);
      }
      if (succ) {
	Intervals_completed(succ);
      }
      DBG("Before vertex_stack while loop:\n");
      DBGCALL(print_vertex_stack(vertex_stack, vertex_stack_top));
      vint popped_vertex_id;
      do {
	popped_vertex_id = *(--vertex_stack_top);
	DBG("    POP VERTEX " VFMT " from vertex_stack[" VFMT "]\n", popped_vertex_id, vertex_stack_top-vertex_stack);
	TC_insert_vertex(tc, popped_vertex_id);
      } while (popped_vertex_id != vertex_id);
      DBG("After vertex_stack while loop:\n");
      DBGCALL(print_vertex_stack(vertex_stack, vertex_stack_top));
      TC_scc_completed(tc);
      *(scc_stack_top++) = scc_id;
    }
  }
  DBG("EXIT visit(" VFMT ") returns " VFMT "\n", vertex_id, lowest);
  return lowest;
}

TC* stacktc (Digraph *g)
{
  vint vertex_count = g->vertex_count;
  DBG("stacktc\n");
  tc = TC_new(g);
  vertex_stack = vertex_stack_top = new_vint_table(vertex_count, -1);
  depth_first_numbers = new_vint_table(vertex_count, -1);
  depth_first_number_counter = 0;
  vertex_table = g->vertex_table;
  vertex_id_to_scc_id_table = tc->vertex_id_to_scc_id_table;
  scc_table = tc->scc_table;
  scc_stack = scc_stack_top = new_vint_table(g->edge_count, -1);
  for (vint n = 0; n < vertex_count; n++) {
    visit(n);
  }
  DELETE(depth_first_numbers);
  DELETE(vertex_stack);
  DELETE(scc_stack);
  return tc;
}
