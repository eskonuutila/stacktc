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
// This file implements transitive closure algorithm STACK_TC presented in:
//   E. Nuutila: Efficient transitive closure computation in large digraphs,
//   PhD thesis, Helsinki University of Technology, Laboratory of Information
//   Processing Science, 1995.
//
// There are two variants: one that sorts the component stack before processing 
// the components and another that does not sort the components.
//
//
// Ifdefs used directly in this file:
//  
//   COLLECT_STACK_METRICS: if non-zero node stack and control stack metrics are computed
//
//   CPU_TIME_TEST: if non-zero, applies multiple times the algorithm and gets 
//   the minimum cpu time.
//
//   SORT_COMPONENT_STACK: if non-zero, component stack is sorted
//
//   UNIQUE_SUCCESSOR_SET_TYPE: use only one set type
//
//   USE_CHAIN_SETS, USE_INTERVAL_SETS, USE_LIST_BIT_SETS: use them
//

#include <math.h>
#include <values.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "util.h"
#include "Graph.h"
#include "Metric.h"

extern double cpu_usec;

extern MetricsList *predefined_metrics;
extern Metric* traversal_depth_metric;
extern Metric* component_stack_depth_metric;
extern Metric* component_stack_scan_metric;
extern Metric* component_stack_skip_metric;
extern Metric* time_metric;

typedef struct {
  int nid;
  int *child;
  int *stop_child;
  int lowest;
  int *component_stack_position;
  int self_loop_p;
} recursion_stack_entry;

static recursion_stack_entry *recursion_stack_global;
static int *node_stack_global;
static int *depth_first_numbers_global;
static int *component_stack_global;

#define RECURSION_PUSH(rtopi, nid, child, stop_child, lowest, component_stack_position, self_loop_p) { \
      recursion_stack[rtopi].nid = nid;					\
      recursion_stack[rtopi].child = child;				\
      recursion_stack[rtopi].stop_child = stop_child;			\
      recursion_stack[rtopi].lowest = lowest;				\
      recursion_stack[rtopi].component_stack_position = component_stack_position; \
      recursion_stack[rtopi].self_loop_p = self_loop_p;			\
      rtopi++;								\
}

#define RECURSION_POP(nid, child, stop_child, component_stack_position, self_loop_p, dfn) { \
 rtopi--;								\
 nid = recursion_stack[rtopi].nid;					\
 child = recursion_stack[rtopi].child;					\
 stop_child = recursion_stack[rtopi].stop_child;			\
 lowest = recursion_stack[rtopi].lowest;				\
 component_stack_position = recursion_stack[rtopi].component_stack_position; \
 self_loop_p = recursion_stack[rtopi].self_loop_p;			\
 dfn = depth_first_numbers[nid];                                        \
}

void stacktc_NAME (Graph *g)
{
  recursion_stack_entry *recursion_stack;
  recursion_stack = recursion_stack_global;
  int rtopi = 0;
  int *node_stack, *node_stack_top;
  node_stack = node_stack_top = node_stack_global;
  int *depth_first_numbers = depth_first_numbers_global;
  int depth_first_number_counter = 0;
  Node *nodes = g->nodes;
  Node *node;
  int dfn, lowest, child_value;
  int *child, *stop_child;
  int node_count = g->node_count;
  SCCSet *components = g->components;

  int *node_id_to_scc_id_table;
  node_id_to_scc_id_table = components->node_id_to_scc_id_table();
  SCC **scc_table = components->scc_table();
  int *component_stack, *component_stack_top;
  component_stack = component_stack_top = component_stack_global;
  for (int n = 0; n < node_count; n++) { BARF((stderr, "main loop %d: visited(%d) = %d\n", n, n, depth_first_numbers[n]));
    if (depth_first_numbers[n] >= 0) { BARF((stderr, "%d already visited\n", n));
      continue;
    }
    int nid = n;
  enter_node: BARF((stderr, "enter_node %d\n", nid)); BARF((stderr, "pushing %d onto node_stack[%d]\n", nid, node_stack_top - node_stack));
    *(node_stack_top++) = nid;
    depth_first_numbers[nid] = dfn = lowest = depth_first_number_counter++; BARF((stderr, "dfn[%d] = %d\n", nid, dfn));
    node = nodes + nid;
    child = node->children;
    int self_loop_p = 0;
    int *component_stack_position = component_stack_top;
    if (!child) { BARF((stderr, "%d has no children\n", nid));
      goto new_component;
    }
    stop_child = child + node->edge_count;
  process_child: 
    BARF((stderr, "process_child: nid = %d, dfn = %d, lowest = %d, ", nid, dfn, lowest));
    BARF((stderr, "rtopi = %d, ntop = %d, etop = %d, ", rtopi, node_stack_top - node_stack, component_stack_top - component_stack));
    BARF((stderr, "child = %lx, stop_child = %lx, ", (long)child, (long)stop_child));
    BARF((stderr, "*child = %d, child_value = %d\n", *child, depth_first_numbers[*child]));
    child_value = depth_first_numbers[*child];
    if (child_value > dfn) { BARF((stderr, "Forward edge (%d, %d), ignore\n", nid, *child));
      goto get_next_child;
    } else if (child_value >= 0) {
      int child_scc_id = node_id_to_scc_id_table[*child];
      if (child_scc_id >= 0) { BARF((stderr, "Intercomponent cross edge (%d,%d)\npush %d to component_stack[%d]\n",
				     nid, *child, child_scc_id, component_stack_top-component_stack));
	*(component_stack_top++) = child_scc_id;
      } else if (child_value < lowest) { BARF((stderr, "Back edge or intracomponent cross edge (%d,%d)\nlowest = %d\n", nid, *child, child_value));
	lowest = child_value;
      } else if (*child == nid) { BARF((stderr, "Self loop edge (%d,%d)\n", nid, nid));
	self_loop_p = 1;
	goto get_next_child;
      }
    } else {  BARF((stderr, "Tree edge (%d, %d)\npushing %d onto recursion_stack[%d]\n", nid, *child, nid, rtopi));
      RECURSION_PUSH(rtopi, nid, child, stop_child, lowest, component_stack_position, self_loop_p);
      nid = *child;
      goto enter_node;
    }
  get_next_child:  BARF((stderr, "Try to get next child of %d\n", nid));
    if (++child == stop_child) {  BARF((stderr, "All children of %d processed, lowest = %d, dfn = %d\n", nid, lowest, dfn));
      if (lowest == dfn) {
      new_component: BARF((stderr, "Node %d is the component root\n", nid));
	SCC* new_scc = components->create_scc(nid);
	int scc_id = new_scc->scc_id();	BARF((stderr, "generate new component %d, root = %d\n", scc_id, nid));
	int self_insert = self_loop_p || (*(node_stack_top-1) != nid); BARF((stderr, "self_insert = %d, self_loop = %d\n", self_insert, self_loop_p));
	SuccessorSet* succ = 0;
	int components_in_stack = component_stack_top - component_stack_position; BARF((stderr, "component_stack contains %d adjacent components of %d\n",
											components_in_stack, scc_id));
	if (components_in_stack) { BARF((stderr, "Sort adjacent components\n"));
	  qsort(component_stack_position, components_in_stack, sizeof(int), &cmp_int);
	  succ = new_scc->create_successors();  BARF((stderr, "Creating successor set for component %d\n", scc_id));
	  int comp_id = *(--component_stack_top); BARF((stderr, "Popping %d from component_stack[%d]\n", comp_id, component_stack_top-component_stack));
	  BARF((stderr, "Scanning adjacent components of %d on component_stack\n", scc_id));
	  do { BARF((stderr, "Adjacent component %d: trying to insert\n", comp_id));
	      if (!(succ->insert(comp_id))) { BARF((stderr, "Component %d not already in Succ[%d], inserting," " unioning SUcc[%d]\n", comp_id, scc_id, comp_id));
		successor_set_union(succ, scc_table[comp_id]->successors(), successor_set_type);
	      } else { BARF((stderr, "Component %d  already in Succ[%d]\n", comp_id, scc_id));
	      }
	      BARF((stderr, "Popping equal components\n"));
	    pop_equals:
	      if (component_stack_top == component_stack_position) { BARF((stderr, "All adjacent components of %d processed\n", scc_id));
		break;
	      } else { 
		int next_comp_id = *(--component_stack_top);
		if (next_comp_id == comp_id) { BARF((stderr, "Popping duplicate %d from component_stack[%d]\n", comp_id, component_stack_top-component_stack));
		  goto pop_equals;
		} else { BARF((stderr, "No more duplicates\n"));
		  comp_id = next_comp_id; BARF((stderr, "Popping %d from component_stack[%d]\n", comp_id, component_stack_top-component_stack));
		}
	      }
	    } while (1);
	  BARF((stderr, "Adjacent components of %d on component_stack scanned\n", scc_id));
	  if (self_insert) { BARF((stderr, "Inserting %d to its own successor set\n", scc_id));
	    succ->insert(scc_id);
	  }
	    ((IntervalSet*)succ)->complete();
	} else {
	  if (self_insert) { BARF((stderr, "Creating successor set for component %d\n", scc_id));
	    succ = new_scc->create_successors();
	    succ->insert(scc_id); BARF((stderr, "Inserting %d to its own successor set\n", scc_id));
	    ((IntervalSet*)succ)->complete();
	  }
	}
	BARF((stderr, "node_stack while loop:\n"));
	int popped_nid;
	do {
	  popped_nid = *(--node_stack_top); BARF((stderr, "Popping %d from node_stack[%d]\n", popped_nid, node_stack_top-node_stack));
	  components->insert_node_to_current_scc(popped_nid);
	} while (popped_nid != nid);
	BARF((stderr, "end node_stack while loop\n"));
	components->scc_completed();
	if (rtopi == 0) { BARF((stderr, "Returning from %d: Spanning tree done\n", nid));
	  goto main_loop_next;
	} else { BARF((stderr, "Returning from %d to %d\n", nid, recursion_stack[rtopi].nid));
	  RECURSION_POP(nid, child, stop_child, lowest, component_stack_position, dfn, self_loop_p, dfn);
	  *(component_stack_top++) = scc_id;
	}
      } else {
	int prev_lowest;
	RECURSION_POP(nid, child, stop_child, prev_lowest, component_stack_position, dfn, self_loop_p, dfn);
	if (lowest > prev_lowest) {
	  lowest = prev_lowest;
	}
      }
      goto get_next_child;
    }
    goto process_child;
  main_loop_next: BARF((stderr, "main_loop_next\n"));
  }
}

extern int looping;

void closure_stacktc_NAME (Graph* g) {
  g->method = METHOD_NAME;
  predefined_metrics->reset();
  g->save_results();
  g->components = NEW(SCCSet(g->node_count, successor_set_type));
  depth_first_numbers_global = new_int_table(g->node_count, -1);
  node_stack_global = new_int_table(g->node_count, -1);
  component_stack_global = new_int_table(g->edge_count, -1);
  recursion_stack_global = NEW(recursion_stack_entry[g->node_count]);
  if (GetTimeOfDay(&time1) < 0) printf_abort("Could not get time\n");
  stacktc_NAME(g);
  if (GetTimeOfDay(&time2) < 0) printf_abort("Could not get time\n");
  cpu_usec = usec_sub(&time2, &time1);
  g->components->set_closure_computed_p();
  g->components->set_components_computed_p();
  DELETE(depth_first_numbers_global);
  DELETE(node_stack_global);
  DELETE(component_stack_global);
  DELETE_ARRAY(recursion_stack_global);
  ALWAYS_INSERT_SAMPLE(time_metric, cpu_usec);
}
