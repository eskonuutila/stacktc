#ifndef _graph_h
#define _graph_h

#include <stdio.h>
#include "config.h"
#include "util.h"
#include "allocs.h"
#include "succ.h"

enum scc_similarity_type { scc_same_component_counts, scc_same_components,
			   scc_same_closures };

enum closure_print_style { closure_print_condensed, closure_print_full,closure_print_structure,
			   closure_print_ranges, closure_print_edges };
typedef struct node_struct {
  int node_id;
  int edge_count;
  int *children;
} *node;

DECLARE_ALLOC_VARIABLES(scc);

typedef struct scc_struct {
  int scc_id;
  int root_node_id;
  int size;
  int *nodes;
  successor_set *successors;
} *scc;

typedef struct scc_set {
  scc **scc_table;
  int scc_cursor;
  int *node_id_to_scc_id_table;
  int *node_table;
  int node_cursor;
  int saved_node_cursor;
  int graph_node_count;
  int components_computed_p;
  int closure_computed_p;
  int graph_modified_p;
};

enum graph_style { graph_dag, graph_cyclic };

class Graph {
  graph_style style;
  double outdegree;
  int node_count;
  int edge_count;
  int localizep;
  int delta;
  int self_loops_allowed_p;
  int random_edge_order_p;
  int random_node_order_p;
  int *edges;
  int seed;
  node *nodes;
  scc_set *components;
  scc_set *saved_components;
};

#endif

