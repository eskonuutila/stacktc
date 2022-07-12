/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: digraph.c

  The implementation of the digraph.
  =============================================================================
*/

#include "digraph.h"
#include "scc.h"

int edge_cmp(const void* a1, const void* a2) {
  EDGE *e1 = (EDGE*)a1;
  EDGE *e2 = (EDGE*)a2;
  if (e1->from < e2->from) return -1;
  else if (e1->from > e2->from) return 1;
  else if (e1->to < e2->from) return -1;
  else if (e1->to > e2->from) return 1;
  else return 0;
}

/* Reading the input graph from a two-column csv file with header naming the fields */

Digraph *digraph_read(char *input_file) {
  vint vertex_count;
  vint edge_count;
  vint max_vertex;
  vint edge_index;
  vint from_vertex, to_vertex, prev_from_vertex;
  vint vi;
  char first_line[101];
  fpos_t second_line_start;
  EDGE *edges;
  Vertex *vertex_table;
  vint *children;
  Digraph *result;
  int got;
  char line1[101];

  FILE *input;
  if (input_file == NULL || !strcmp(input_file, "-")) {
      input = stdin;
  } else if (!(input = fopen(input_file, "r"))) {
    fprintf(stderr, "Cannot open input file %s\n", input_file);
    exit(1);
  }

  DBG("input: %ld\n", ftell(input));
  if (fscanf(input, "%s\n", line1) != 1) {
    fprintf(stderr, "Could not read line\n");
    exit(1);
  } else {
    char *comma = strchr(line1, ',');
    char *field1, *field2;
    if (comma != NULL) {
      field1 = line1;
      *comma = (char)0;
      field2 = comma + 1;
      DBG("'%s', '%s'\n", field1, field2);
    } else {
      fprintf(stderr, "Could not read first line\n");
      return NULL;
    }
  }
  fgetpos(input, &second_line_start);
  edge_count = 0;
  char* linefmt = VFMT "," VFMT "\n";
  while ((got = fscanf(input, linefmt, &from_vertex, &to_vertex)) == 2) {
    DBG("Got 1 %d\n", got);
    if (from_vertex < 0 || to_vertex < 0) {
      fprintf(stderr, "Illegal edge " VFMT "," VFMT "!\n", from_vertex, to_vertex);
      return NULL;
    }
    edge_count++;
    DBG("edge_count = " VFMT "\n", edge_count);
  }
  DBG("Last got 1 %d\n", got);
  if (!feof(input)) {
    fprintf(stderr, VFMT " edges read, not at end of file, last got %d!\n", edge_count, got);
    return NULL;
  }
  DBG("Reading " VFMT " edges\n", edge_count);
  edges = NEWN(EDGE, edge_count);
  fsetpos(input, &second_line_start);
  edge_index = 0;
  max_vertex = 0;
  while ((got = fscanf(input, linefmt, &(edges[edge_index].from), &(edges[edge_index].to)) == 2)) {
    DBG("Got 2 %d\n", got);
    if (edges[edge_index].from > max_vertex) {
      max_vertex = edges[edge_index].from;
    }
    if (edges[edge_index].to > max_vertex) {
      max_vertex = edges[edge_index].to;
    }
    edge_index++;
    DBG("edge_index = " VFMT "\n", edge_index);
  }
  DBG("Last got 2 %d\n", got);
  if (edge_index != edge_count) {
    fprintf(stderr, "Something wrong " VFMT " == edge_index != edge_countedge_index != " VFMT "\n", edge_index, edge_count);
  }
  fclose(input);
  DBG("Sorting edges\n");
  qsort((void*)edges, edge_count, sizeof(EDGE), &edge_cmp);
  vertex_count = max_vertex + 1;
  DBG("Creating " VFMT " vertex_table\n", vertex_count);
  vertex_table =  NEWN(Vertex,vertex_count);
  children = NEWN(vint, edge_count);
  edge_index = 0;
  for (vi = 0; vi < vertex_count; vi++) {
    vertex_table[vi].vertex_id = vi;
    vertex_table[vi].children = children + edge_index;
    vertex_table[vi].outdegree = 0;
    while (edges[edge_index].from == vi) {
      children[edge_index] = edges[edge_index].to;
      vertex_table[vi].outdegree++;
      edge_index++;
    }
    DBG("Vertex " VFMT ", " VFMT " edges\n", vi, vertex_table[vi].outdegree);
  }
  result = NEW(Digraph);
  result->vertex_count = vertex_count;
  result->edge_count = edge_count;
  result->edge_table = children;
  result->vertex_table = vertex_table;
  return result;
}

/* ==== Converting the transitive closure back to a digraph ==== */
Digraph *tc_to_digraph(TC *tc) {
  Digraph *result = NEW(Digraph);
  vint vertex_count = tc->vertex_count;
  vint scc_count = tc->scc_count;
  vint edge_count = 0;
  vint *edges;
  vint i, j, k, l, m;
  vint edge_index;
  vint *to_table = new_vint_table(vertex_count, -1);
  /* DBG("tc_to_digraph " VFMT " vertices " VFMT " components\n", vertex_count, scc_count); */
  result->vertex_count = vertex_count;
  result->vertex_table = NEWN(Vertex, vertex_count);
  for (i = 0; i < scc_count; i++) {
    edge_count += tc->scc_table[i]->vertex_count*SCC_successor_vertex_count(tc, i);
  }
  /* DBG(VFMT " edges in tc\n", edge_count); */
  edges = new_vint_table(edge_count, -1);
  result->edge_count = edge_count;
  edge_index = 0;
  for (i = 0; i < scc_count; i++) {
    SCC *scc_from = tc->scc_table[i];
    Intervals *succ = scc_from->successors;
    vint to_table_index = 0;
    if (succ != NULL) {
      /* DBG("SCC " VFMT ", " VFMT " intervals\n", i, succ->interval_count); */
      for (j = 0; j < succ->interval_count; j++) {
	Interval *iv = &(succ->interval_table[j]);
	for (l = iv->low; l <= iv->high; l++) {
	  /* DBG("interval " VFMT ".." VFMT ", scc " VFMT "\n", iv->low, iv->high, l); */
	  SCC *scc_to = tc->scc_table[l];
	  for (m = 0; m < scc_to->vertex_count; m++) {
	    /* DBG("copying edge to vertex " VFMT "\n", scc_to->vertex_table[m]); */
	    to_table[to_table_index++] = scc_to->vertex_table[m];
	  }
	}
      }
    }
    qsort(to_table, to_table_index, sizeof(vint), &cmp_vint);
    for (j = 0; j < scc_from->vertex_count; j++) {
      /* DBG("copying to edges[" VFMT ".." VFMT "]\n", edge_index, edge_index + to_table_index-1); */
      memcpy(edges+edge_index, to_table, to_table_index*sizeof(vint));
      k = scc_from->vertex_table[j];
      result->vertex_table[k].vertex_id = k;
      result->vertex_table[k].outdegree = to_table_index;
      result->vertex_table[k].children = edges+edge_index;
      edge_index += to_table_index;
    }
  }
  /* DBG("returning digraph(" VFMT "," VFMT ")\n", result->vertex_count, result->edge_count); */
  return result;
}

/* ==== Converting a digraph into a matrix ==== */

Matrix *digraph_to_matrix(Digraph *this) {
  vint n = this->vertex_count;
  Matrix *matrix = NEW(Matrix);
  matrix->n = n;
  /* DBG("digraph_to_matrix (" VFMT ", " VFMT ")\n", n, this->edge_count); */
  vint *elements = matrix->elements = (vint*)calloc(n*n, sizeof(vint));
  vint i, j;
  for (i = 0; i < n; i++) {
    Vertex* v = &(this->vertex_table[i]);
    for (j = 0; j < v->outdegree; j++) {
      /* DBG("edge " VFMT "->" VFMT "\n", v->vertex_id, v->children[j]); */
      elements[v->vertex_id*n + v->children[j]] = 1;
    }
  }
  return matrix;
}

