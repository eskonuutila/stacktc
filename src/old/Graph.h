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
// This file declares graph classes Node, SCC, SCCSet, and Graph.
//
// Ifdefs used directly in this file
//  
//   USE_NO_SETS: use no sets!
//
//   USE_CHAIN_SETS: use chain sets!
//

#ifndef _graph_h
#define _graph_h

#include <stdio.h>
#include "config.h"
#include "util.h"
#include "allocs.h"
#include "SuccessorSet.h"

enum SCCSimilarityType { SCCSameComponentCounts, SCCSameComponents,
			 SCCSameClosures };

#ifndef USE_NO_SETS

enum ClosurePrintStyle { ClosurePrintCondensed, ClosurePrintFull,ClosurePrintStructure,
		       ClosurePrintRanges, ClosurePrintEdges };
#endif

class Node {
 public:
  int node_id;
  int edge_count;
  int *children;
};

DECLARE_ALLOC_VARIABLES(SCC);

class SCC {
  int v_scc_id;
  int v_root_node_id;
  int v_size;
  int *v_nodes;
#ifndef USE_NO_SETS
  SuccessorSet *v_successors;
#endif
  DECLARE_ALLOC_ROUTINES(SCC);
 public:
  /* Callee owns */
  SCC(int sccid, int rootnodeid, int* nodes) {
    v_scc_id = sccid;
    v_root_node_id = rootnodeid;
    v_size = 0;
    v_nodes = nodes;
#ifndef USE_NO_SETS
    v_successors = 0;
#endif
  }
  ~SCC();
  int scc_id() { return v_scc_id; }
  int size() { return v_size; }
  void set_size(int s) { v_size = s; }
  int root_node_id() { return v_root_node_id; }
  /* Callee owns */
  int *node_table() { return v_nodes; }
#ifndef USE_NO_SETS
  /* Callee owns */
  SuccessorSet *create_successors();
  /* Callee owns */
  void set_successors(SuccessorSet* succ) { v_successors = succ; }
  /* Callee owns */
  SuccessorSet* successors() { return v_successors; }
#endif
  friend class SCCSet;
};

class SCCSet {
#ifdef USE_CHAIN_SETS
  Chains *v_chains;
#endif
#if defined(USE_INTERVAL_SETS) || defined(USE_CHAIN_SETS)
  BlockAllocator *v_block_allocator;
#endif
  SCC **v_scc_table;
  int v_scc_cursor;
  int *v_node_id_to_scc_id_table;
  int *v_node_table;
  int v_node_cursor;
  int v_saved_node_cursor;
  int v_graph_node_count;
  int v_components_computed_p;
#ifndef USE_NO_SETS
  int v_closure_computed_p;
  SetType v_successor_set_type;
  int v_successor_sets_contain_nodes_p;
#endif
  int v_graph_modified_p;
 public:
  SCCSet(int node_count
#ifndef USE_NO_SETS
	 , SetType successor_set_type
#endif
	 );
  ~SCCSet();
  /* Callee owns */
  int *node_id_to_scc_id_table() { return v_node_id_to_scc_id_table; }
  /* Callee owns */
  int *node_table() { return v_node_table; }
  /* Callee owns */
  SCC **scc_table() { return v_scc_table; }
  void set_node_cursor(int c) { v_node_cursor = c; }
  void set_scc_cursor(int c) { v_scc_cursor = c; }
  int graph_modified_p() { return v_graph_modified_p; }
  void set_graph_modified_p(int val) { v_graph_modified_p = val; }
  int component_size_count();
  void check();
  int scc_count() { return v_scc_cursor + 1; }
  int components_computed_p() { return v_components_computed_p; }
  void set_components_computed_p() { v_components_computed_p = 1; }
#ifndef USE_NO_SETS
  int closure_computed_p() { return v_closure_computed_p; }
  void set_closure_computed_p() { v_closure_computed_p = 1; }
  SetType successor_set_type() { return v_successor_set_type; }
  int successor_sets_contain_nodes_p() {
    return v_successor_sets_contain_nodes_p;
  }
  void set_successor_sets_contain_nodes_p(int val) {
    v_successor_sets_contain_nodes_p = val;
  }
  int edge_count();
#endif
  /* Callee owns */
  SCC* create_scc(int root_id) {
    v_scc_cursor++;
    return (v_scc_table[v_scc_cursor] = NEW(SCC(v_scc_cursor, root_id, 
						v_node_table+v_node_cursor)));
  }
  void scc_completed() {
    v_scc_table[v_scc_cursor]->set_size(v_node_cursor - v_saved_node_cursor);
    v_saved_node_cursor = v_node_cursor;
  }    
  /* Callee owns */
  SCC* scc_id_to_scc(int scc_id) { return v_scc_table[scc_id]; }
  void insert_node_to_current_scc(int node_id) {
    v_node_table[v_node_cursor++] = node_id;
    v_node_id_to_scc_id_table[node_id] = v_scc_cursor;
  }
  int node_id_to_scc_id(int node_id) { 
    return v_node_id_to_scc_id_table[node_id];
  }
  /* Callee owns */
  SCC* node_id_to_scc(int node_id) { 
    return v_scc_table[v_node_id_to_scc_id_table[node_id]];
  }
  /* Caller owns */
  int similar(SCCSet* other, SCCSimilarityType stype, FILE* output=0, int abortp=1);
  /* Caller owns */
#ifndef USE_NO_SETS
  int similar_by_node(SCCSet* other, FILE* output=0, int abortp=1);
#endif
  /* Caller owns */
  void print_components(FILE *output, char* open="{", char* close="}",
			char* node_sep=", ", char* component_sep=",\n");
  /* Caller owns */
  void print_component_size_distribution(FILE *output,
					 char* open="{", char* close="}",
					 char* sep=",");
  /* Caller owns */
#ifndef USE_NO_SETS
  void print_closure(FILE *output, 
		     ClosurePrintStyle pstyle=ClosurePrintCondensed,
		     char* open="{", char* close="}", 
		     char* node_sep=", ", char* component_sep=",\n");
#endif
};

enum GraphStyle { GraphDAG, GraphCyclic };

class Graph {
 public:
  static int save_components_p;
  ~Graph();
  GraphStyle style;
  char *method;
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
  Node *nodes;
  SCCSet *components;
  SCCSet *saved_components;
  void clear_results();
  void save_results();
  /* Caller owns */
  void print_graph_parameters(FILE* output);
  /* Caller owns */
  void print_graph_outdegree_distribution(FILE* output,
					  char* open="{", char* close="}", 
					  char* sep=",");
  /* Caller owns */
  void print_graph_indegree_distribution(FILE* input,
					 char* open="{", char* close="}", 
					 char* sep=",");
  /* Caller owns */
  void print_graph(FILE *output, 
		   char* init="",
		   char* edge_list_open="{", char* edge_list_close="}\n",
		   char* new_from_sep=",\n",
		   char* edge_sep=",\n",
		   char* edge_open="{", char* edge_close="}", char *edge_comma=", ");
  void print_graph_readable(FILE *output);
  /* Caller owns */
  void sum_outdegrees(int *(*outdegrees), int *max_outdegree, 
		      int *outdegree_table_size);
  /* Caller owns */
  void sum_indegrees(int *(*indegrees), int *max_indegree, 
		      int *indegree_table_size);
#ifndef USE_NO_SETS
  /* Caller owns */
  void partition_to_small_large(unsigned char *succ_large_p,
				unsigned char *pred_large_p);
#endif
  void check();
};

/* Caller owns */
Graph *random_graph(GraphStyle style, 
		    int node_count,
		    double outdegree,
		    int seed, 
		    int localizep=0,
		    int delta=0,
		    int self_loops_allowed_p=0,
		    int random_edge_order_p=1, 
		    int random_node_order_p=1);

/* Caller owns */
Graph *random_graph2(GraphStyle style, 
		    int node_count,
		    double outdegree,
		    int seed, 
		    int localizep=0,
		    int delta=0,
		    int self_loops_allowed_p=0,
		    int random_edge_order_p=1, 
		    int random_node_order_p=1);
#endif
