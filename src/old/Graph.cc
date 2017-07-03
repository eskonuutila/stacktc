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
// This file implements graph classes.
//
// Ifdefs used directly in this file:
//  
//   USE_CHAIN_SETS, USE_INTERVAL_SETS, USE_NO_SETS: use them (or don't)
//

#include <minmax.h>
//#ifndef
//#else
//static inline double min(double a, double b) {
//  if (a < b) return a;
//  else return b;
//}
//#endif
#include <stdlib.h>
#include "config.h"
#include "util.h"
#include "allocs.h"
#include "Metric.h"
#include "Graph.h"

extern MetricsList *predefined_metrics;
extern Metric *time_metric;

DEFINE_ALLOC_ROUTINES(SCC, 1024, 128);

SCC::~SCC () {
#ifndef USE_NO_SETS
  if (v_successors) DELETE(v_successors);
#endif
}

#ifndef USE_NO_SETS
SuccessorSet *SCC::create_successors() {
  return (v_successors = create_successor_set(successor_set_type));
}

#endif

SCCSet::SCCSet(int node_count
#ifndef USE_NO_SETS
	       , SetType succ_set_type
#endif
)
{
  v_graph_node_count = node_count;
  v_node_id_to_scc_id_table = new_int_table(node_count, -1);
  v_scc_table = NEW(SCC*[node_count]);
  v_scc_cursor = -1;
  v_node_table = new_int_table(node_count, -1);
  v_node_cursor = v_saved_node_cursor = 0;
  v_graph_modified_p = 0;
  v_components_computed_p = 0;
#ifndef USE_NO_SETS
  v_closure_computed_p = 0;
  v_successor_set_type = succ_set_type;
  v_successor_sets_contain_nodes_p = 0;
#ifdef USE_CHAIN_SETS
  if (succ_set_type == TypeChainSet) {
    ChainSet::initialize_closure(node_count);
    v_block_allocator = ChainSet::s_allocator;
    v_chains = NEW(Chains(node_count));
  }
#endif
#ifdef USE_INTERVAL_SETS
  if (succ_set_type == TypeIntervalSet) {
    IntervalSet::initialize_closure(node_count);
    v_block_allocator = IntervalSet::s_allocator;
  }
#endif
#endif
}

SCCSet::~SCCSet() 
{
  DELETE(v_node_id_to_scc_id_table);
  for (int i = 0; i <= v_scc_cursor; i++)
    DELETE(v_scc_table[i]);
  DELETE(v_scc_table);
  DELETE(v_node_table);
#ifdef USE_CHAIN_SETS
  if (v_successor_set_type == TypeChainSet) {
    DELETE(v_block_allocator);
    if (v_chains) {
      DELETE(v_chains);
    }
  }
#endif
#ifdef USE_INTERVAL_SETS
  if (v_successor_set_type == TypeIntervalSet) {
    DELETE(v_block_allocator);
  }
#endif
}

#ifndef USE_NO_SETS
int SCCSet::similar_by_node(SCCSet* other, FILE* output, int abortp) {
  METRICS_OFF;
  Assert(successor_sets_contain_nodes_p());
  int i;
  if (other->successor_sets_contain_nodes_p()) {
    for (i = 0; i < v_node_cursor; i++) {
      SuccessorSet *this_set = node_id_to_scc(i)->successors();
      SuccessorSet *other_set = other->node_id_to_scc(i)->successors();
      if (!successor_set_similar(this_set,
				 v_successor_set_type,
				 other_set,
				 other->v_successor_set_type)) {
	if (output)
	  fprintf(output, "Successor sets different in component %d\n", i);
	if (abortp)
	  abort();
	else {	
	  METRICS_RESET;
	  return  0;
	}
      }
    }
  } else {
    int *members = new_int_table(v_graph_node_count, -1);
    for (i = 0; i < v_graph_node_count; i++) {
      SuccessorSet *set_with_nodes = 
	node_id_to_scc(i)->successors();
      SuccessorSet *set_with_components = 
	other->node_id_to_scc(i)->successors();
      if (set_with_nodes == 0 || set_with_components == 0)
	if (set_with_nodes != set_with_components) {
	  if (output)
	    fprintf(output, "Successor sets different in component %d\n", i);
	  if (abortp)
	    abort();
	  else {
	    METRICS_RESET;
	    return 0;
	  }
	} else 
	  continue;
      int scc_id;
      int node_id;
      SuccessorSetIter* comp_iter = set_with_components->iter();
      int prev = v_graph_node_count;
      int node_set_size = 0;
      while ((scc_id = comp_iter->next()) >= 0) {
	SCC *scc = other->scc_id_to_scc(scc_id);
	int size = scc->size();
	node_set_size += size;
	for (int nd = 0; nd < size; nd++) {
	  node_id = scc->v_nodes[nd];
	  members[node_id] = prev;
	  prev = node_id;
	}
      }
      if (set_with_nodes->size() != node_set_size) {
	if (output)
	  fprintf(output, "Successor set sizes %d and %d different in component %d\n",
		  set_with_nodes->size(), node_set_size, i);
	if (abortp)
	  abort();
	else {
	  METRICS_RESET;
	  return 0;
	}
      }
      SuccessorSetIter* node_iter = set_with_nodes->iter();
      while ((node_id = node_iter->next()) >= 0) {
	if (members[node_id] < 0) {
	  if (output)
	    fprintf(output, "Successor sets different in component %d\n"
		    "Node %d missing\n", i, node_id);
	  if (abortp)
	    abort();
	  else {
	    METRICS_RESET;
	    return 0;
	  }
	}
      }
      while (prev != v_graph_node_count) {
	node_id = prev;
	prev = members[node_id];
	members[node_id] = -1;
      }
      set_with_components->delete_iter(comp_iter);
      set_with_nodes->delete_iter(node_iter);
    }
    DELETE(members);
  }
  METRICS_RESET;
  return 1;
}
#endif

int SCCSet::similar(SCCSet* other, SCCSimilarityType stype, FILE* output,
		    int abortp) {
  int i;
  if (this == NULL || other == NULL) {
    if (output)
      fprintf(output, "? Two SCCSets needed - cannot compare\n");
    return 0;
  }
  if (other->graph_modified_p())
    fprintf(stderr, "You have probably first applied BTC and then some other algorithm. You will lose!\n");
#ifndef USE_NO_SETS
  if (!components_computed_p()) {
    return similar_by_node(other, output, abortp);
  } else if (!other->components_computed_p()) {
    return other->similar_by_node(this, output, abortp);
  }
#endif
  METRICS_OFF;
  if (scc_count() != other->scc_count()) {
    if (output)
      fprintf(output, "Different numbers of components: %d, %d\n", 
	      scc_count(), other->scc_count());

    if (abortp)
      abort();
    else {
      METRICS_RESET;
      return 0;
    }
  }  
  for (int c = 0; c < scc_count(); c++) {
    if (scc_id_to_scc(c)->root_node_id() != 
	other->scc_id_to_scc(c)->root_node_id()) {
      if (output)
	fprintf(output, "root of scc %d != root of other scc %d\n", c, c);
      if (abortp)
	abort();
      else {
	METRICS_RESET;
	return 0;
      }
    }
  }
  if (stype == SCCSameComponentCounts) {
    METRICS_RESET;
    return 1;
  }
  Assert(v_node_cursor == other->v_node_cursor);
  for (i = 0; i < v_node_cursor; i++)
    if (node_id_to_scc_id(i) != other->node_id_to_scc_id(i)) {
      if (output)
	fprintf(output, "node %d in scc %d and scc %d\n", i,
		node_id_to_scc_id(i), other->node_id_to_scc_id(i));
      if (abortp)
	abort();
      else {
	METRICS_RESET;
	return 0;
      }
    }  
  if (stype == SCCSameComponents) {
    METRICS_RESET;
    return 1;
  }
#ifndef USE_NO_SETS
  if (!closure_computed_p()) {
    if (other->closure_computed_p()) {
      if (output)
	fprintf(output, "Closure not computed for the first argument\n");
      METRICS_RESET;
      return 0;
    } 
  } else if (!other->closure_computed_p()) {
    if (output)
      fprintf(output, "Closure not computed for the second argument\n");
    METRICS_RESET;
    return 0;
  } else {
    if (v_successor_sets_contain_nodes_p == other->v_successor_sets_contain_nodes_p) {
      for (i = 0; i < scc_count(); i++) {
	if (!successor_set_similar(scc_id_to_scc(i)->successors(),
				   v_successor_set_type,
				   other->scc_id_to_scc(i)->successors(),
				   other->v_successor_set_type)) {
	  if (output)
	    fprintf(output, "Successor sets different in component %d\n", i);
	  if (abortp)
	    abort();
	  else {	
	    METRICS_RESET;
	    return  0;
	  }
	}
      }
    } else {
      int this_nodes = v_successor_sets_contain_nodes_p;
      SCCSet *sccs_with_nodes, *sccs_with_components;
      if (this_nodes) {
	sccs_with_nodes = this;
	sccs_with_components = other;
      } else {
	sccs_with_nodes = other;
	sccs_with_components = this;
      }
      int max_index = v_graph_node_count;
      int *members = new_int_table(max_index, -1);
      for (i = 0; i < scc_count(); i++) {
	SuccessorSet *set_with_nodes = 
	  sccs_with_nodes->scc_id_to_scc(i)->successors();
	SuccessorSet *set_with_components = 
	  sccs_with_components->scc_id_to_scc(i)->successors();
	if (set_with_nodes == 0 || set_with_components == 0)
	  if (set_with_nodes != set_with_components) {
	    if (output)
	      fprintf(output, "Successor sets different in component %d\n", i);
	    if (abortp)
	      abort();
	    else {
	      METRICS_RESET;
	      return 0;
	    }
	  } else 
	    continue;
	int scc_id;
	int node_id;
	SuccessorSetIter* comp_iter = set_with_components->iter();
	int prev = max_index;
	int node_set_size = 0;
	while ((scc_id = comp_iter->next()) >= 0) {
	  SCC *scc = sccs_with_components->scc_id_to_scc(scc_id);
	  int size = scc->size();
	  node_set_size += size;
	  for (int nd = 0; nd < size; nd++) {
	    node_id = scc->v_nodes[nd];
	    members[node_id] = prev;
	    prev = node_id;
	  }
	}
	if (set_with_nodes->size() != node_set_size) {
	  if (output)
	    fprintf(output, "Successor set sizes %d and %d different in component %d\n",
		    set_with_nodes->size(), node_set_size, i);
	  if (abortp)
	    abort();
	  else {
	    METRICS_RESET;
	    return 0;
	  }
	}
	SuccessorSetIter* node_iter = set_with_nodes->iter();
	while ((node_id = node_iter->next()) >= 0) {
	  if (members[node_id] < 0) {
	    if (output)
	      fprintf(output, "Successor sets different in component %d\n"
		      "Node %d missing\n", i, node_id);
	    if (abortp)
	      abort();
	    else {
	      METRICS_RESET;
	      return 0;
	    }
	  }
	}
	while (prev != max_index) {
	  node_id = prev;
	  prev = members[node_id];
	  members[node_id] = -1;
	}
	set_with_components->delete_iter(comp_iter);
	set_with_nodes->delete_iter(node_iter);
      }
      DELETE(members);
    }
  }
#endif
  METRICS_RESET;
/*
  if (output)
    fprintf(output, "SCCSets are similar\n");
*/
  return 1;
}

void SCCSet::print_component_size_distribution(FILE *output,
					       char* open, char* close,
					       char* sep) {
  int i;
  int *distribution = new_int_table(v_node_cursor+1, 0);
  for (i = 0; i <= v_scc_cursor; i++) {
    int index = v_scc_table[i]->size();
    Assert(index >= 1 && index <= v_node_cursor);
    distribution[index]++;
  }  
  fprintf(output, open);
  int first = 1;
  for (i = 1; i <= v_node_cursor; i++)
    if (distribution[i] > 0)
      if (first) {
	fprintf(output, "%s%d%s%d%s", open, i, sep, distribution[i], close);
	first = 0;
      } else
	fprintf(output, "%s%s%d%s%d%s", sep, open, i, sep, distribution[i], 
		close);
  fprintf(output, "%s\n", close);
}

int SCCSet::component_size_count() {
  int i;
  int *distribution = new_int_table(v_node_cursor+1, 0);
  int size_count = 0;
  for (i = 0; i <= v_scc_cursor; i++) {
    int index = v_scc_table[i]->size();
    Assert(index >= 1 && index <= v_node_cursor);
    if (!distribution[index])
      size_count++;
    distribution[index]++;
  }  
  return size_count;
}

void SCCSet::print_components(FILE *output, char* open, char* close,
			      char* node_sep, char* component_sep)
{
  int i, j;
  
  fprintf(output, open);
  int *nodes = 0, *prev_nodes = 0, size = 0, prev_size = 0;
  for (i = 0; i <= v_scc_cursor; i++) {
    SCC* scc = v_scc_table[i];
    if (i > 0) {
      prev_nodes = nodes;
      prev_size = size;
      fprintf(output, component_sep);
    }
    nodes = scc->node_table();
    size = scc->size();
    Assert(i == 0 || nodes == prev_nodes + prev_size);
    fprintf(output, open);
    int root_found_p = 0;
    for (j = 0; j < size; j++) {
      if (j == 0)
	fprintf(output, "%d", nodes[j]);
      else
	fprintf(output, "%s%d", node_sep, nodes[j]);
      if (nodes[j] == scc->root_node_id())
	root_found_p = 1;
    }
    if (!root_found_p)
      printf_abort("Root node %d missing from component %d (size %d)\n",
		   scc->root_node_id(), scc->scc_id(), scc->size());
    fprintf(output, close);
  }
  fprintf(output, "%s\n", close);
}

#ifndef USE_NO_SETS
void SCCSet::print_closure(FILE *output, 
			   ClosurePrintStyle pstyle,
			   char* open, char* close, 
			   char* node_sep, char* component_sep)
{
  int i, j, k;
  
  Assert(closure_computed_p());
  Assert(pstyle != ClosurePrintRanges
	 || v_successor_set_type == TypeRangeSet
	 || v_successor_set_type == TypeIntervalSet);
  int flatp;
  if (pstyle == ClosurePrintRanges) {
    pstyle = ClosurePrintStructure;
    flatp = 1;
  } else
    flatp = 0;
  if (pstyle == ClosurePrintFull || pstyle == ClosurePrintEdges) {
    if (pstyle == ClosurePrintEdges)
      // Nasty cludge: n*n edges
      fprintf(output, "%d %d\n", v_graph_node_count,
	      v_graph_node_count*v_graph_node_count);
    else
      fprintf(output, open);
    int *set_node_table = new_int_table(v_graph_node_count, 0);
    for (i = 0; i < v_node_cursor; i++) {
      if (i > 0 && pstyle == ClosurePrintFull) fprintf(output, component_sep);
      int scc_id = node_id_to_scc_id(i);
      SCC* scc = scc_id_to_scc(scc_id);
      Assert(scc->scc_id() == scc_id);
      SuccessorSet* succ = scc->successors();
      Assert(succ || scc->size() == 1);
      if (pstyle == ClosurePrintFull) fprintf(output, open);
      if (succ) {
	SuccessorSetIter* iter = succ->iter();
	int node_index = 0;
	while ((j = iter->next()) >= 0) {
	  if (!successor_sets_contain_nodes_p()) {
	    SCC* j_scc = scc_id_to_scc(j);
	    Assert(j_scc->scc_id() == j);
	    int root_found_p = 0;
	    int *nodes = j_scc->node_table();
	    for (k = 0; k < j_scc->size(); k++) {
	      set_node_table[node_index++] = nodes[k];
	      if (nodes[k] == j_scc->root_node_id())
		root_found_p = 1;
	    }
	    if (!root_found_p)
	      printf_abort("Root node %d missing from component %d (size %d)\n",
			   j_scc->root_node_id(), j_scc->scc_id(), j_scc->size());
	  } else
	    set_node_table[node_index++] = j;
	}
	succ->delete_iter(iter);
	qsort(set_node_table, node_index, sizeof(int), &cmp_int);
	if (pstyle == ClosurePrintFull) {
	  int firstp = 1;
	  for (int ni = 0; ni < node_index; ni++) {
	    if (firstp) {
	      firstp = 0;
	      fprintf(output, "%d", set_node_table[ni]);
	    } else
	      fprintf(output, "%s%d", node_sep, set_node_table[ni]);
	  }
	} else {
	  for (int ni = 0; ni < node_index; ni++)
	    fprintf(output, "(%d %d)\n", i, set_node_table[ni]);
	}
      }
      if (pstyle == ClosurePrintFull) fprintf(output, close);
    }
    DELETE(set_node_table);
    if (pstyle == ClosurePrintFull) fprintf(output, "%s\n", close);
  } else {
    fprintf(output, open);
    for (i = 0; i <= v_scc_cursor; i++) {
      SCC* scc = v_scc_table[i];
      Assert(scc->scc_id() == i);
      SuccessorSet* succ = scc->successors();
      Assert(succ || scc->size() == 1);
      if (i > 0)
	fprintf(output, component_sep);
      if (pstyle == ClosurePrintCondensed) {
	fprintf(output, open);
	if (succ) {
	  SuccessorSetIter* iter = succ->iter();
	  int firstp = 1;
	  while ((j = iter->next()) >= 0)
	    if (firstp) {
	      firstp = 0;
	      fprintf(output, "%d", j);
	    } else
	      fprintf(output, "%s%d", node_sep, j);
	  succ->delete_iter(iter);
	}
	fprintf(output, close);
      } else if (succ)
	succ->print_structure(output, flatp);
      else
	fprintf(output, "{}");
    }
    fprintf(output, "%s\n", close);
  }
}

#endif

void SCCSet::check() {
#ifndef USE_NO_SETS
  if (closure_computed_p()) {
    int *nodes = 0, *prev_nodes = 0, size = 0, prev_size = 0;
    for (int i = 0; i <= v_scc_cursor; i++) {
      SCC* scc = v_scc_table[i];
      Assert(scc->scc_id() == i);
      Assert(scc->successors() || scc->size() == 1);
      if (i > 0) {
	prev_nodes = nodes;
	prev_size = size;
      } else
	Assert(scc->node_table() == v_node_table);
      nodes = scc->node_table();
      size = scc->size();
      Assert(i == 0 || nodes == prev_nodes + prev_size);
      int root_found_p = 0;
      for (int j = 0; j < size; j++)
	if (nodes[j] == scc->root_node_id())
	  root_found_p = 1;
      Assert(root_found_p);
    }
    Assert(v_node_table + v_node_cursor == nodes + size);
  }
#endif
}

int Graph::save_components_p = 1;

Graph::~Graph()
{ 
  if (edges != NULL) DELETE(edges);
  if (nodes != NULL) DELETE_ARRAY(nodes);
  if (components != NULL) DELETE(components);
  if (saved_components != NULL) DELETE(saved_components);
}

void Graph::save_results()
{
  if (save_components_p) {
    if (saved_components != NULL) DELETE(saved_components);
    saved_components = components;
    components = NULL;
  } else {
    if (components) {
      DELETE(components);
      components = NULL;
    }
  }
}

void Graph::clear_results()
{
  if (components != NULL) DELETE(components);
  if (saved_components != NULL) DELETE(saved_components);
  saved_components = NULL;
  components = NULL;
}

void initialize_random_geometric(double p, int seed);
int random_geometric();
int random_uniform(int low, int high);

extern int random_value;

extern double cpu_usec;

Graph *random_graph2(GraphStyle style,
		     int node_count,
		     double outdegree,
		     int seed,
		     int localizep,
		     int delta,
		     int self_loops_allowed_p,
		     int random_edge_order_p,
		     int random_node_order_p)
{
  Node *nodes;
  int *edges;
  int node_index, node_index_mapped;
  int edge_index;
  int random_counter;
  Graph *result;
  int target_area_width;
  int edge_allocation_count;
  int *node_order_map;

  struct timeval time1, time2;
  DefineTimezone;

  predefined_metrics->reset();

  int tv = GetTimeOfDay(&time1);
  if (tv < 0)
    printf_abort("Could not get time\n");
  if (node_count < 0) {
    fprintf(stderr, "n must be non-negative\n");
    return 0;
  }
  if (seed == 0) {
    fprintf(stderr, "seed must be non-zero\n");
    return 0;
  }
  random_value = seed;
  nodes = NEW(Node[node_count]);
  node_order_map = new_int_table(node_count, -1);
  for (node_index = 0; node_index < node_count; node_index++)
    node_order_map[node_index] = node_index;
  for (node_index = node_count - 1; node_index > 0; node_index--) {
    /* We call the random numbe generator even when the node order is not randomized.
       This guarantees that we get isomorphic graph. */
    int random_index = random_uniform(0, node_index); 
    if (random_node_order_p) {
      int aux = node_order_map[random_index];
      node_order_map[random_index] = node_order_map[node_index];
      node_order_map[node_index] = aux;
    }
  }
  if (style == GraphCyclic) {
    if (localizep) {
      if (2*delta >= node_count) {
	fprintf(stderr, "2*delta must be less than node_count\n");
	DELETE(nodes);
	DELETE(node_order_map);
	return 0;
      }
      target_area_width = 2*delta + (self_loops_allowed_p ? 1 : 0);
    } else {
      delta = node_count / 2;
      target_area_width = node_count - (self_loops_allowed_p ? 0 : 1);
    }
  } else {
    if (self_loops_allowed_p) {
      fprintf(stderr, "DAG cannot contain self loops!\n");
      DELETE(nodes);
      DELETE(node_order_map);
      return 0;
    }
    if (localizep) {
      if (delta >= node_count) {
	fprintf(stderr, "delta must be less than n\n");
	DELETE(nodes);
	DELETE(node_order_map);
	return 0;
      }
    } else {
      delta = node_count - 1;
    }
    target_area_width = delta;
  }
  edge_allocation_count = (int)(ceil(node_count * outdegree * 2));
  if (edge_allocation_count) {
    edges = new_int_table(edge_allocation_count, -1);
    initialize_random_geometric((target_area_width > 0
				 ? min(outdegree/target_area_width,1.0)
				 : 0.0), random_value);
  } else
    edges = 0;
  edge_index = 0;
  for (node_index = 0; node_index < node_count; node_index++) {
    if (style == GraphDAG && node_index + target_area_width == node_count)
      target_area_width--;
    node_index_mapped = node_order_map[node_index];
    nodes[node_index_mapped].node_id = node_index_mapped;
//    fprintf(stderr, "node_index = %d, node_index_mapped = %d\n", 
//	    node_index, node_index_mapped);
    int saved_edge_index = edge_index;
    if (target_area_width > 0 && outdegree != 0.0) {
      for (random_counter = -1 + random_geometric(); random_counter < target_area_width;
	   random_counter += random_geometric()) {
	if (edge_index == edge_allocation_count) {
	  fprintf(stderr, "reallocate edge_table\n");
	  edge_allocation_count *= 2;
	  int *new_edges = new_int_table(edge_allocation_count, -1);
	  for (int cursor = 0; cursor < edge_index; cursor++)
	    new_edges[cursor] = edges[cursor];
	  DELETE(edges);
	  edges = new_edges;
	}
	if (style == GraphCyclic) {
	  int j = (node_index + random_counter - delta
		   + (!self_loops_allowed_p && random_counter >= delta ? 1 : 0) 
		   + node_count) % node_count;
	  edges[edge_index++] = node_order_map[j];
	} else {
	  edges[edge_index++] = node_order_map[random_counter + node_index + 1];
	}
//	fprintf(stderr, "edges[%d] = %d\n", edge_index-1, edges[edge_index-1]);
      }
    }
    nodes[node_index_mapped].edge_count = edge_index - saved_edge_index;
  }
  int *edge_ptr = edges;
  for (node_index = 0; node_index < node_count; node_index++) {
    node_index_mapped = node_order_map[node_index];
    int edge_count = nodes[node_index_mapped].edge_count;
    nodes[node_index_mapped].children = (int*)(edge_count > 0 ? edge_ptr : NULL);
    edge_ptr += edge_count;
  }
  if (outdegree == 0.0 || node_count == 0)
    Assert(edge_index == 0);
  for (node_index = 0; node_index < node_count; node_index++) {
    int *node_edges = nodes[node_index].children;
    for (int i = nodes[node_index].edge_count - 1; i > 0; i--) {
      int target = random_uniform(0, i);
      if (random_edge_order_p) {
	int aux = node_edges[target];
	node_edges[target] = node_edges[i];
	node_edges[i] = aux;
      }
    }  
  }
  DELETE(node_order_map);
  result = NEW(Graph);
  result->method = "Generated2";
  result->seed = seed;
  result->style = style;
  result->outdegree = outdegree;
  result->node_count = node_count;
  result->edge_count = edge_index;
  result->localizep = localizep;
  result->delta = delta;
  result->self_loops_allowed_p = self_loops_allowed_p;
  result->random_edge_order_p = random_edge_order_p;
  result->random_node_order_p = random_node_order_p;
  result->nodes = nodes;
  result->edges = edges;
  result->components = NULL;
  result->saved_components = NULL;
  if (GetTimeOfDay(&time2) < 0)
    printf_abort("Could not get time\n");
  cpu_usec = usec_sub(&time2, &time1);
  ALWAYS_INSERT_SAMPLE(time_metric, cpu_usec);
  return result;
}

Graph *random_graph(GraphStyle style, 
		    int node_count, double outdegree, int seed, 
		    int localizep,
		    int delta,
		    int self_loops_allowed_p,
		    int random_edge_order_p, 
		    int random_node_order_p)
{
  Node *nodes;
  int *edges;
  int node_index, node_index_mapped;
  int edge_index, prev_edge_index;
  int random_counter;
  Graph *result;
  int target_area_width;
  int edge_allocation_count;
  int *node_order_map;

  struct timeval time1, time2;
  DefineTimezone;

  predefined_metrics->reset();

  int tv = GetTimeOfDay(&time1);
  if (tv < 0)
    printf_abort("Could not get time\n");
  if (node_count < 0) {
    fprintf(stderr, "n must be non-negative\n");
    return 0;
  }
  if (seed == 0) {
    fprintf(stderr, "seed must be non-zero\n");
    return 0;
  }
  random_value = seed;
  nodes = NEW(Node[node_count]);
  node_order_map = new_int_table(node_count, -1);
  for (node_index = 0; node_index < node_count; node_index++)
    node_order_map[node_index] = node_index;
  if (random_node_order_p) {
    for (node_index = node_count - 1; node_index >= 0; node_index--) {
      int random_index = random_uniform(0, node_index);
      int aux = node_order_map[random_index];
      node_order_map[random_index] = node_order_map[node_index];
      node_order_map[node_index] = aux;
    }
  }
  if (style == GraphDAG) {
    printf_exit("Dags not working!\n");
    if (self_loops_allowed_p) {
      fprintf(stderr, "DAG cannot contain self loops!\n");
      DELETE(nodes);
      DELETE(node_order_map);
      return 0;
    }
    if (localizep) {
      if (delta >= node_count) {
	fprintf(stderr, "delta must be less than n\n");
	DELETE(nodes);
	DELETE(node_order_map);
	return 0;
      }
      target_area_width = delta;
    } else 
      target_area_width = node_count - 1;
    edge_allocation_count = irint(node_count * outdegree * 2);
    if (edge_allocation_count)
      edges = new_int_table(edge_allocation_count, -1);
    else
      edges = 0;
    edge_index = prev_edge_index = 0;
    random_counter = 0;
    node_index = 0;
    node_index_mapped = node_order_map[node_index];
    nodes[node_index_mapped].node_id = node_index_mapped;
    if (outdegree != 0.0)
      initialize_random_geometric(min(outdegree / target_area_width, 1.0), 
				  random_value);
    while (1) {
      if (outdegree != 0.0) {
	int rg = random_geometric();
	random_counter = random_counter + rg;
      }
      while (outdegree == 0.0 || random_counter > target_area_width) {
	if (edge_index == prev_edge_index) {
	  nodes[node_index_mapped].children = NULL;
	  nodes[node_index_mapped].edge_count = 0;
	} else {
	  nodes[node_index_mapped].children = edges + prev_edge_index;
	  nodes[node_index_mapped].edge_count = edge_index - prev_edge_index;
	  prev_edge_index = edge_index;
	}
	if (++node_index == node_count) goto end;
	node_index_mapped = node_order_map[node_index];
	nodes[node_index_mapped].node_id = node_index_mapped;
	if (outdegree != 0.0) {
	  random_counter = random_counter - target_area_width;
	  if (!localizep || node_index >= node_count - delta) {
	    target_area_width--;
	    initialize_random_geometric(min(outdegree / target_area_width,1.0), 
					random_value);
	  }
	}
      }
      if (edge_index == edge_allocation_count) {
	edge_allocation_count *= 2;
	int *new_edges = new_int_table(edge_allocation_count, -1);
	for (int cursor = 0; cursor < edge_index; cursor++)
	  new_edges[cursor] = edges[cursor];
	DELETE(edges);
	edges = new_edges;
      }
      edges[edge_index++] = node_order_map[random_counter + node_index];
    }
  } else /* style == GraphCyclic */ {
    if (localizep) { 
      target_area_width = 2*delta;
      if (target_area_width + 1 > node_count) {
	fprintf(stderr, "2*delta must be less than node_count\n");
	DELETE(nodes);
	DELETE(node_order_map);
	return 0;
      }
    } else {
      delta = node_count / 2; 
      target_area_width = node_count - 1; 
    }
    if (self_loops_allowed_p) target_area_width++;
    edge_allocation_count = (int)(ceil(node_count * outdegree * 2));
//    fprintf(stderr, "Edge_a_c = %d\n", edge_allocation_count);
    if (edge_allocation_count)
      edges = new_int_table(edge_allocation_count, -1);
    else
      edges = 0;
    edge_index = prev_edge_index = 0;
    random_counter = -1;
    node_index = 0;
    node_index_mapped = node_order_map[node_index];
    nodes[node_index_mapped].node_id = node_index_mapped;
    if (outdegree != 0.0)
      initialize_random_geometric((target_area_width > 0
				   ? min(outdegree/target_area_width,1.0)
				   : 0.0), random_value);
    while (1) {
      if (outdegree != 0.0) {
	int rg = random_geometric();
	random_counter = random_counter + rg;
      }
      while (outdegree == 0.0 || random_counter > target_area_width) {
	if (edge_index == prev_edge_index) {
	  nodes[node_index_mapped].children = NULL;
	  nodes[node_index_mapped].edge_count = 0;
	} else {
	  nodes[node_index_mapped].children = edges + prev_edge_index;
	  nodes[node_index_mapped].edge_count = edge_index - prev_edge_index;
	  prev_edge_index = edge_index;
	}
	if (++node_index == node_count) goto end;
	node_index_mapped = node_order_map[node_index];
	nodes[node_index_mapped].node_id = node_index_mapped;
	if (outdegree != 0.0)
	  random_counter = random_counter - target_area_width;
      }
      if (edge_index == edge_allocation_count) {
	edge_allocation_count *= 2;
    fprintf(stderr, "Edge_a_c = %d\n", edge_allocation_count);
	int *new_edges = new_int_table(edge_allocation_count, -1);
	for (int cursor = 0; cursor < edge_index; cursor++)
	  new_edges[cursor] = edges[cursor];
	DELETE(edges);
	edges = new_edges;
      }
      int j = (node_index + random_counter - delta
	       + (!self_loops_allowed_p && random_counter >= delta ? 1 : 0) 
	       + node_count) % node_count;
      edges[edge_index++] = node_order_map[j];
    }
  }
 end:
  if (outdegree == 0.0)
    Assert(edge_index == 0);
  else if (random_edge_order_p) {
    for (node_index = 0; node_index < node_count; node_index++) {
      int *node_edges = nodes[node_index].children;
      for (int index = nodes[node_index].edge_count - 1; index >= 0; index--) {
	int random_index = random_uniform(0, index);
	int aux = node_edges[random_index];
	node_edges[random_index] = node_edges[index];
	node_edges[index] = aux;
      }
    }    
  } else {
    for (node_index = 0; node_index < node_count; node_index++) {
      qsort(nodes[node_index].children,nodes[node_index].edge_count, 
	    sizeof(int),&cmp_int);
    }
  }
  DELETE(node_order_map);
  result = NEW(Graph);
  result->method = "Generated";
  result->seed = seed;
  result->style = style;
  result->outdegree = outdegree;
  result->node_count = node_count;
  result->edge_count = edge_index;
  result->localizep = localizep;
  result->delta = delta;
  result->self_loops_allowed_p = self_loops_allowed_p;
  result->random_edge_order_p = random_edge_order_p;
  result->random_node_order_p = random_node_order_p;
  result->nodes = nodes;
  result->edges = edges;
  result->components = NULL;
  result->saved_components = NULL;
  if (GetTimeOfDay(&time2) < 0)
    printf_abort("Could not get time\n");
  cpu_usec = usec_sub(&time2, &time1);
  ALWAYS_INSERT_SAMPLE(time_metric, cpu_usec);
  return result;
}

Graph* read_graph(FILE* input) {
  int node_count;
  int edge_count;
  int node_cursor;
  int edge_cursor, saved_edge_cursor = 0;
  int from_node, to_node, prev_from_node;
  struct timeval time1, time2;
  DefineTimezone;

  predefined_metrics->reset();

  if (GetTimeOfDay(&time1) < 0)
    printf_abort("Could not get time\n");
  if (fscanf(input, "%d %d\n", &node_count, &edge_count) != 2) {
    fprintf(stderr, "Illegal input file\n");
    return 0;
  }
  Node *nodes =  NEW( Node[node_count]);
  int *edges = new_int_table(edge_count, -1);
  for (node_cursor = 0; node_cursor < node_count; node_cursor++) {
    nodes[node_cursor].node_id = node_cursor;
    nodes[node_cursor].edge_count = 0;
    nodes[node_cursor].children = NULL;
  }
  Graph *result;
  prev_from_node = -1;
  for (edge_cursor = 0; edge_cursor < edge_count; edge_cursor++) {
    if (fscanf(input, "(%d %d)\n", &from_node, &to_node) != 2 ||
	from_node < 0 || from_node >= node_count || 
	to_node < 0 || to_node >= node_count) {
      
      DELETE(nodes);
      DELETE(edges);
      fprintf(stderr, "Illegal input file (edge %d)\n", edge_cursor);
      return 0;
    }
    if (from_node != prev_from_node) {
      if (prev_from_node >= 0)
	nodes[prev_from_node].edge_count = edge_cursor - saved_edge_cursor;
      if (nodes[from_node].edge_count != 0) {
	DELETE(nodes);
	DELETE(edges);
	fprintf(stderr, "Illegal input file (edge %d out of order)\n", edge_cursor);
	return 0;
      }
      nodes[from_node].children = &(edges[edge_cursor]);
      saved_edge_cursor = edge_cursor;
    }
    edges[edge_cursor] = to_node;
    prev_from_node = from_node;
  }
  if (prev_from_node >= 0)
    nodes[prev_from_node].edge_count = edge_cursor - saved_edge_cursor;
  if (!feof(input))
    fprintf(stderr, "%d edges read, not at end of file!\n", edge_count);
  result = NEW(Graph);
  result->style = GraphCyclic;
  result->method = "Read";
  result->outdegree = (node_count > 0 ? ((double)edge_count)/((double)node_count) : 0);
  result->node_count = node_count;
  result->edge_count = edge_cursor;
  result->nodes = nodes;
  result->edges = edges;
  result->components = NULL;
  result->saved_components = NULL;
  return result;
  if (GetTimeOfDay(&time2) < 0)
    printf_abort("Could not get time\n");
  cpu_usec = usec_sub(&time2, &time1);
  ALWAYS_INSERT_SAMPLE(time_metric, cpu_usec);
}

void Graph::print_graph_parameters(FILE* output) {
  fprintf(output, "method %s\n", method);
  fprintf(output, "seed %d\n", seed);
  fprintf(output, "style ");
  if (style == GraphDAG) fprintf(output, "DAG\n");
  else fprintf(output, "Cyclic\n");
  fprintf(output, "node_count %d\n", node_count);
  fprintf(output, "outdegree %g\n", outdegree);
  fprintf(output, "localizep %d\n", localizep);
  fprintf(output, "delta %d\n", delta);
  fprintf(output, "self_loops_allowed_p %d\n", self_loops_allowed_p);
  fprintf(output, "random_edge_order_p %d\n", random_edge_order_p);
  fprintf(output, "random_node_order_p %d\n", random_node_order_p);
  fprintf(output, "edge_count %d\n", edge_count);
}

void Graph::print_graph(FILE *output,
			char* init,
			char* edge_list_open, char* edge_list_close,
			char* new_from_sep,
			char* edge_sep,
			char* edge_open, char* edge_close, char *edge_comma)
{
  int i, j, first_node, first_edge;
  
  if (!this) {
    fprintf(output, "? No graph\n");
    return;
  }
  fprintf(output, init, node_count, edge_count);
  fprintf(output, edge_list_open);
  first_node = 1;
  for (i = 0; i < node_count; i++) {
    if (first_node)
      first_node = 0;
    else
      fprintf(output, new_from_sep);
    first_edge = 1;
    for (j = 0; j < nodes[i].edge_count; j++) {
      if (first_edge)
	first_edge = 0;
      else
	fprintf(output, edge_sep);
      fprintf(output, "%s%d%s%d%s", 
	      edge_open, i, edge_comma, nodes[i].children[j], edge_close);
    }
  }
  fprintf(output, edge_list_close);
}

void Graph::print_graph_readable(FILE *output) {
  print_graph(output, "%d %d\n", "", "\n", "\n", "\n", "(", ")", " ");
}

void Graph::check() {
  int child_id;

  int *prev_edge_ptr = edges;
  for (int edge = 0; edge < edge_count; edge++) {
    child_id = edges[edge];
    if (child_id < 0 || child_id >= node_count) abort();
  }
  for (int node = 0; node < node_count; node++) {
    if (nodes[node].node_id != node) abort();
    int ec = nodes[node].edge_count;
    if (ec == 0) {
      if (nodes[node].children != NULL) abort();
    } else {
      if (!(nodes[node].children)) abort();
      if (!random_node_order_p) {
	if (nodes[node].children != prev_edge_ptr) abort();
	prev_edge_ptr = nodes[node].children + ec;
	if (prev_edge_ptr > edges + edge_count) abort();
      }
      for (int child = 0; child < ec; child++) {
	child_id = nodes[node].children[child];
	if (child_id < 0 || child_id >= node_count) abort();
	if (!self_loops_allowed_p && child_id == node) abort();
	if (style == GraphCyclic) {
	  if (localizep && !random_node_order_p) {
	    int low = node - delta;
	    int high = node + delta;
	    if (low < 0)
	      if (high < node_count) {
		if (!(child_id <= high && child_id >= low + node_count))
		  abort();
	      } else 
		abort();
	    else if (high >= node_count) {
	      if (!(child_id >= low && child_id <= high - node_count))
		abort();
	    } else {
	      if (!(child_id >= low && child_id <= high))
		abort();
	    }
	  }
	} else {
	  if (child_id == node) abort();
	  if (!random_node_order_p) {
	    if (child_id < node) abort();
	    if (localizep)
	      if (!(child_id <= node + delta)) abort();
	  }
	}
      }
    }
  }
}

void Graph::sum_outdegrees(int *(*outdegrees), int *max_outdegree, 
			   int *outdegree_table_size) {
  for (int node_cursor = 0; node_cursor < node_count; node_cursor++) {
    int node_outdegree = nodes[node_cursor].edge_count;
    if (node_outdegree >= *outdegree_table_size) {
      int new_size = *outdegree_table_size * 2;
      int *new_table = new_int_table(new_size, 0);
      for (int i = 0; i < *outdegree_table_size; i++)
	new_table[i] = (*outdegrees)[i];
      *outdegrees = new_table;
      *outdegree_table_size = new_size;
    } 
    (*outdegrees)[node_outdegree]++;
    if (node_outdegree > *max_outdegree)
      *max_outdegree = node_outdegree;
  }
}

void Graph::print_graph_outdegree_distribution(FILE *output,
					       char* open, char* close,
					       char* sep) {
  int table_size = node_count + 1;
  int *outdegrees = new_int_table(node_count+1, 0);
  int max_outdegree = 0;
  sum_outdegrees(&outdegrees, &max_outdegree, &table_size);
  fprintf(output, open);
  int firstp = 1;
  for (int i = 0; i <= max_outdegree; i++)
    if (firstp) {
      firstp = 0;
      fprintf(output, "%s%d%s%d%s", open, i, sep, outdegrees[i], close);
    } else
      fprintf(output, "%s%s%d%s%d%s", sep, open, i, sep, outdegrees[i], close);
  fprintf(output, "%s\n", close);
  DELETE_ARRAY(outdegrees);
}

void Graph::sum_indegrees(int *(*indegrees), int *max_indegree, 
			  int *indegree_table_size) {
  int *node_indegrees = new_int_table(node_count, 0);
  for (int edge_cursor = 0; edge_cursor < edge_count; edge_cursor++)
    node_indegrees[edges[edge_cursor]]++;
  for (int node_cursor = 0; node_cursor < node_count; node_cursor++) {
    int node_indegree = node_indegrees[node_cursor];
    if (node_indegree >= *indegree_table_size) {
      int new_size = *indegree_table_size * 2;
      int *new_table = new_int_table(new_size, 0);
      for (int i = 0; i < *indegree_table_size; i++)
	new_table[i] = (*indegrees)[i];
      *indegrees = new_table;
      *indegree_table_size = new_size;
    } 
    (*indegrees)[node_indegree]++;
    if (node_indegree > *max_indegree)
      *max_indegree = node_indegree;
  }
  DELETE_ARRAY(node_indegrees);
}

void Graph::print_graph_indegree_distribution(FILE *output,
					       char* open, char* close,
					       char* sep) {
  int table_size = node_count + 1;
  int *indegrees = new_int_table(node_count+1, 0);
  int max_indegree = 0;
  sum_indegrees(&indegrees, &max_indegree, &table_size);
  fprintf(output, open);
  int firstp = 1;
  for (int i = 0; i <= max_indegree; i++)
    if (firstp) {
      firstp = 0;
      fprintf(output, "%s%d%s%d%s", open, i, sep, indegrees[i], close);
    } else
      fprintf(output, "%s%s%d%s%d%s", sep, open, i, sep, indegrees[i], close);
  fprintf(output, "%s\n", close);
  DELETE_ARRAY(indegrees);
}

#ifndef USE_NO_SETS
void Graph::partition_to_small_large(unsigned char *succ_large_p,
				     unsigned char *pred_large_p) {
  Assert(components);
  Assert(components->components_computed_p());
  Assert(components->closure_computed_p());
  Assert(!(components->successor_sets_contain_nodes_p()));
  int i;
  for (i = 0; i < node_count; i++)
    succ_large_p[i] = pred_large_p[i] = 0;
  int giant_scc_id = -1;
  int giant_scc_size = -1;
  int scc_count = components->scc_count();
  for (i = 0; i < scc_count; i++) {
    int size = components->scc_id_to_scc(i)->size();
    if (size > giant_scc_size) {
      giant_scc_id = i;
      giant_scc_size = size;
    }
  }
  for (i = 0; i < scc_count; i++) {
    SCC *scc = components->scc_id_to_scc(i);
    SuccessorSet *succ = scc->successors();
    if (succ && succ->find(giant_scc_id)) {
      int scc_size = scc->size();
      int *node_table = scc->node_table();
      for (int j = 0; j < scc_size; j++)
	succ_large_p[node_table[j]] = 1;
    }
  }
  SuccessorSet *giant_succ =
    components->scc_id_to_scc(giant_scc_id)->successors();
  if (giant_succ) {
    SuccessorSetIter *iter = giant_succ->iter();
    int scc_id;
    while ((scc_id = iter->next()) >= 0) {
      SCC *scc = components->scc_id_to_scc(scc_id);
      int scc_size = scc->size();
      int *node_table = scc->node_table();
      for (int j = 0; j < scc_size; j++)
	pred_large_p[node_table[j]] = 1;
    }
    giant_succ->delete_iter(iter);
  }
}
#endif
