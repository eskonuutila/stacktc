#include <minmax.h>
#include <stdlib.h>
#include "config.h"
#include "util.h"
#include "allocs.h"
#include "Graph.h"

DEFINE_ALLOC_ROUTINES(strong_component, 1024, 128);

strong_component* strong_component_new(int sccid, int rootnodeid, int* nodes) {
  strong_component* this = NEW(strong_component*);
  this->scc_id = sccid;
  this->root_node_id = rootnodeid;
  this->size = 0;
  this->nodes = nodes;
  this->successors = 0;
}

void strong_component_delete(this scc) {
  if (this->successors) DELETE(this->successors);
}

successor_set *strong_component_create_successors() {
  return (v_successors = create_successor_set(successor_set_type));
}

strong_component_set* strong_component_set_new(int node_count)
{
  this = NEW(strong_component_set);
  this->graph_node_count = node_count;
  this->node_id_to_scc_id_table = new_int_table(node_count, -1);
  this->scc_table = NEW((scc*)[node_count]);
  this->scc_cursor = -1;
  this->node_table = new_int_table(node_count, -1);
  this->node_cursor = this->saved_node_cursor = 0;
  this->graph_modified_p = 0;
  this->components_computed_p = 0;
  this->closure_computed_p = 0;
  interval_set_initialize_closure(node_count);
  this->block_allocator = interval_set_s_allocator;
}

void strong_component_set_delete(strong_component* this_set) {
  DELETE(this->node_id_to_scc_id_table);
  for (int i = 0; i <= this->scc_cursor; i++)
    strong_component_delete(this->scc_table[i]);
  DELETE(this->scc_table);
  DELETE(this->node_table);
  DELETE(this->block_allocator);
}

strong_component* create_strong_component(strong_component_set* this, int root_id) {
  v_scc_cursor++;
  return (v_scc_table[v_scc_cursor] = NEW(strong_component(v_scc_cursor, root_id, 
							   v_node_table+v_node_cursor)));
}

void strong_component_set_scc_completed(strong_component_set* this, ) {
  v_scc_table[v_scc_cursor]->set_size(v_node_cursor - v_saved_node_cursor);
  v_saved_node_cursor = v_node_cursor;
}    

strong_component* strong_component_set_scc_id_to_scc(strong_component_set* this, int scc_id) { return v_scc_table[scc_id]; }

int strong_component_set_similar(strong_component_set* this, strong_component_set* other, scc_similarity_type stype, FILE* output, int abortp) {
  Assert(this->closure_computed_p && other->closure_computed_p);
  if (scc_count() != other->scc_count()) {
    if (output) fprintf(output, "Different numbers of components: %d, %d\n", scc_count(), other->scc_count());
    RETURN_ZERO_OR_ABORT(abortp);
  }
  for (int c = 0; c < scc_count(); c++) {
    if (scc_id_to_scc(c)->root_node_id() != other->scc_id_to_scc(c)->root_node_id()) {
      if (output) fprintf(output, "root of scc %d != root of other scc %d\n", c, c);
      RETURN_ZERO_OR_ABORT(abortp);
    }
  }
  if (stype == SCCSameComponentCounts) {
    return 1;
  }
  Assert(this->node_cursor == other->node_cursor);
  for (i = 0; i < this->node_cursor; i++)
    if (strong_component_set_node_id_to_scc_id(this, i) != other->strong_component_set_node_node_id_to_scc_id(other,i)) {
      if (output) fprintf(output, "node %d in scc %d and scc %d\n",
			  i, strong_component_set_node_id_to_scc_id(this, i), other->strong_component_set_node_node_id_to_scc_id(other,i));
      RETURN_ZERO_OR_ABORT(abortp);
    }
  if (stype == SCCSameComponents) {
    return 1;
  }
  for (i = 0; i < scc_count(); i++) {
    if (!successor_set_similar(scc_id_to_scc(i)->successors(), other->scc_id_to_scc(i)->successors())) {
      if (output) fprintf(output, "Successor sets different in component %d\n", i);
      RETURN_ZERO_OR_ABORT(abortp);
    }
  }
  return 1;
}


void print_closure(FILE *output, 
		   closure_print_style pstyle=closure_print_condensed,
		   char* open="{", char* close="}", 
		   char* node_sep=", ", char* component_sep=",\n");

#define RETURN_ZERO_OR_ABORT(x) if (x) abort() else return 0

void strong_component_set_print_components(strong_component_set* this, FILE *output, char* open, char* close, char* node_sep, char* component_sep)
{
  int i, j;
  
  fprintf(output, open);
  int *nodes = 0, *prev_nodes = 0, size = 0, prev_size = 0;
  for (i = 0; i <= this->scc_cursor; i++) {
    strong_component* scc = this->scc_table[i];
    if (i > 0) {
      prev_nodes = nodes;
      prev_size = size;
      fprintf(output, component_sep);
    }
    nodes = scc->node_table;
    size = scc->size;
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
      printf_abort("Root node %d missing from component %d (size %d)\n", scc->root_node_id(), scc->scc_id(), scc->size());
    fprintf(output, close);
  }
  fprintf(output, "%s\n", close);
}

void strong_component_set_print_closure(strong_component_set* this, FILE *output, ClosurePrintStyle pstyle, char* open, char* close, char* node_sep, char* component_sep)
{
  int i, j, k;
  
  Assert(closure_computed_p());
  Assert(pstyle != ClosurePrintRanges || this->successor_set_type == TypeRangeSet || this->successor_set_type == Typeinterval_set);
  int flatp;
  if (pstyle == ClosurePrintRanges) {
    pstyle = ClosurePrintStructure;
    flatp = 1;
  } else
    flatp = 0;
  if (pstyle == ClosurePrintFull || pstyle == ClosurePrintEdges) {
    if (pstyle == ClosurePrintEdges)
      // Nasty cludge: n*n edges
      fprintf(output, "%d %d\n", this->graph_node_count, this->graph_node_count*this->graph_node_count);
    else
      fprintf(output, open);
    int *set_node_table = new_int_table(this->graph_node_count, 0);
    for (i = 0; i < this->node_cursor; i++) {
      if (i > 0 && pstyle == ClosurePrintFull) fprintf(output, component_sep);
      int scc_id = strong_component_set_node_node_id_to_scc_id(this, i);
      strong_component* scc = scc_id_to_scc(scc_id);
      Assert(scc->scc_id() == scc_id);
      successor_set* succ = scc->successors();
      Assert(succ || scc->size == 1);
      if (pstyle == ClosurePrintFull) fprintf(output, open);
      if (succ) {
	int node_index = 0;
	int si;
	for (si = 0; si < succ->interval_count; si++) {
	  int j = succ->intervals[si];
	  if (!successor_sets_contain_nodes_p()) {
	    strong_component* j_scc = scc_id_to_scc(j);
	    Assert(j_scc->scc_id() == j);
	    int root_found_p = 0;
	    int *nodes = j_scc->node_table();
	    for (k = 0; k < j_scc->size; k++) {
	      set_node_table[node_index++] = nodes[k];
	      if (nodes[k] == j_scc->root_node_id())
		root_found_p = 1;
	    }
	    if (!root_found_p)
	      printf_abort("Root node %d missing from component %d (size %d)\n",
			   j_scc->root_node_id(), j_scc->scc_id(), j_scc->size);
	  } else
	    set_node_table[node_index++] = j;
	}
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
    for (i = 0; i <= this->scc_cursor; i++) {
      strong_component* scc = this->scc_table[i];
      Assert(scc->scc_id() == i);
      successor_set* succ = scc->successors();
      Assert(succ || scc->size == 1);
      if (i > 0)
	fprintf(output, component_sep);
      if (pstyle == ClosurePrintCondensed) {
	fprintf(output, open);
	if (succ) {
	  int firstp = 1;
	  int si;
	  for (si = 0; si < succ->interval_count; si++) {
	    int j = succ->intervals[si];
	    if (firstp) {
	      firstp = 0;
	      fprintf(output, "%d", j);
	    } else
	      fprintf(output, "%s%d", node_sep, j);
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

void strong_component_set_check(strong_component_set* this) {
  if (this->closure_computed_p) {
    int *nodes = 0, *prev_nodes = 0, size = 0, prev_size = 0;
    for (int i = 0; i <= this->scc_cursor; i++) {
      strong_component* scc = this->scc_table[i];
      Assert(scc->scc_id == i);
      Assert(scc->successors || scc->size == 1);
      if (i > 0) {
	prev_nodes = nodes;
	prev_size = size;
      } else
	Assert(scc->node_table == this->node_table);
      nodes = scc->node_table;
      size = scc->size;
      Assert(i == 0 || nodes == prev_nodes + prev_size);
      int root_found_p = 0;
      for (int j = 0; j < size; j++)
	if (nodes[j] == scc->root_node_id)
	  root_found_p = 1;
      Assert(root_found_p);
    }
    Assert(this->node_table + this->node_cursor == nodes + size);
  }
}

graph* read_graph(FILE* input) {
  int node_count;
  int edge_count;
  int node_cursor;
  int edge_cursor, saved_edge_cursor = 0;
  int from_node, to_node, prev_from_node;

  if (fscanf(input, "%d %d\n", &node_count, &edge_count) != 2) {
    fprintf(stderr, "Illegal input file\n");
    return 0;
  }
  node *nodes =  NEW( Node[node_count]);
  int *edges = new_int_table(edge_count, -1);
  for (node_cursor = 0; node_cursor < node_count; node_cursor++) {
    nodes[node_cursor].node_id = node_cursor;
    nodes[node_cursor].edge_count = 0;
    nodes[node_cursor].children = NULL;
  }
  graph *result;
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
}
