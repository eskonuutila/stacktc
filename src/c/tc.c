/*  -*- Mode: C -*-                                                              */
/*===============================================================================*/
/* Author:  Esko Nuutila (enu@iki.fi)                                            */
/* Date:    2017-06-23                                                           */
/* Licence: MIT                                                                  */
/*                                                                               */
/* Description:                                                                  */
/*                                                                               */
/* This file implements transitive closure algorithm STACK_TC presented in:      */
/*     E. Nuutila: Efficient transitive closure computation in large digraphs,   */
/*     PhD thesis, Helsinki University of Technology, Laboratory of Information  */
/*     Processing Science, 1995.                                                 */
/*                                                                               */
/*     https://github.com/eskonuutila/tc/blob/master/docs/thesis.pdf             */
/*                                                                               */
/*===============================================================================*/

#include "tc.h"

/*==== Global variables ==== */

vint *vertex_stack, *vertex_stack_top;
vint *depth_first_numbers;
vint depth_first_number_counter;
Vertex *vertex_table;
TC *tc;
vint *vertex_id_to_scc_id_table;
SCC **scc_table;
vint *scc_stack, *scc_stack_top;

/* ==== Printing the structs; used in debugging ==== */

static void print_vertex_struct(vint vertex_id) {
  Vertex *vertex = vertex_table + vertex_id;
  vint outdegree = vertex->outdegree;
  vint *children = vertex->children;
  fprintf(stderr, "vertex[" VFMT "] " VFMT " children: ", vertex_id, outdegree);
  for (vint i = 0; i < outdegree; i++) {
    fprintf(stderr, " " VFMT, children[i]);
  }
  fprintf(stderr, "\n");
}


static void print_scc_struct(vint scc_id) {
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


static void print_vertex_stack() {
  vint n = vertex_stack_top - vertex_stack;
  for (vint i = 0; i < n; i++) {
    fprintf(stderr, "    vertex_stack[" VFMT "] = " VFMT "\n", i, vertex_stack[i]);
  }
}

/* ==== Axiliary operations ==== */

/* Allocate and initialize an array of vints */
vint *new_vint_table(vint nelem, vint init) {
  vint *table = NEWN(vint, nelem);
  vint i;
  for (i = 0; i < nelem; i++) {
    table[i] = init;
  }
  return table;
}

/* Compare two vints. Used as a parameter for qsort */
int cmp_vint(const void *a, const void *b) {
  return (*((vint*)a) - *((vint*)b));
}


/* ==== Intervals: representing the result sets of transitive closure ==== */
/*
   An interval set contains a set of non-overlapping continuous sequences of numbers called intervals.
   An interval is represented by its smallest and largest element.

   For example, the set {0, 1, 2, 5, 6, 7, 8, 12} can be represented as the intervals set
   {[0,2], [5,8], [12, 12]}.
   
   As presented in section 4.2 of the thesis, using interval set representation makes transitive
   closure computation much faster and the resulting transitive closure can be represented
   in much smaller space than if we were storing individual vertex of strong component numbers
   in regular sets.

*/

Interval *interval_table_from = 0;
Interval *interval_table_to = 0;

void Intervals_initialize_tc(vint max_ids) {
  interval_table_to = NEWN(Interval, max_ids/2+1);
  interval_table_from = NEWN(Interval, max_ids/2+1);
}

Intervals *Intervals_new() {
  Intervals *this = NEW(Intervals);
  this->interval_count = 0;
  this->interval_table = interval_table_from;
  return this;
}

/* Inserting a number to an interval set. This may extend an existing interval,
   generate a new interval, or do nothing if the number already is in the interval set */
vint Intervals_insert(Intervals *this, vint id) {
  vint min = 0;
  vint max = this->interval_count - 1;
  Interval *ins = this->interval_table;
  if (min > max) {
    ins[0].low = ins[0].high = id;
    this->interval_count = 1;
    return 0;
  } else {
    do {
      vint index = (max + min)/2;
      Interval *elem = &(ins[index]);
      if (id < elem->low)
	max = index - 1;
      else if (id > elem->high)
	min = index + 1;
      else {
	return 1;
      }
    } while (min <= max);
  }
  if (max < 0) {
    if (id == ins[0].low - 1) {
      ins[0].low--;
    } else {
      vint i;
      for (i = this->interval_count - 1; i >= 0; i--)
	ins[i+1] = ins[i];
      ins[0].low = ins[0].high = id;
      this->interval_count++;
    }
  } else if (min == this->interval_count) {
    if (id == ins[min - 1].high + 1)
      ins[min - 1].high++;
    else {
      ins[min].low = ins[min].high = id;
      this->interval_count++;
    }
  } else {
    if (id == ins[max].high + 1) {
      if (id == ins[min].low - 1) {
	ins[max].high = ins[min].high;
	vint i;
	for (i = min; i < this->interval_count - 1; i++)
	  ins[i] = ins[i + 1];
	this->interval_count--;
      } else {
	ins[max].high++;
      }
    } else if (id == ins[min].low - 1) {
      ins[min].low--;
    } else {
      vint i;
      for (i = this->interval_count - 1; i >= min; i--)
	ins[i+1] = ins[i];
      ins[min].low = ins[min].high = id;
      this->interval_count++;
    }
  }
  return 0;
}

/* The union of two interval sets. Note that the result may contain
   a smaller number of intervals than either of the parameters */
void Intervals_union(Intervals *this, Intervals *other) {
  if (!other || other->interval_count == 0) return;
  Interval *result = interval_table_to;
  Interval *ins1 = this->interval_table;
  Interval *ins2 = other->interval_table;
  vint i1 = 0, i2 = 0, i = 0;
  vint max1 = this->interval_count;
  vint max2 = other->interval_count;
  if (max1 == 0)
    goto copy2;
  while (1) {
    if (ins1[i1].high < ins2[i2].low - 1) {
      result[i++] = ins1[i1++];
      if (i1 == max1)
	goto copy2;
    } else if (ins2[i2].high < ins1[i1].low - 1) {
      result[i++] = ins2[i2++];
      if (i2 == max2)
	goto copy1;
    } else {
      if (ins1[i1].low < ins2[i2].low)
	result[i].low = ins1[i1].low;
      else
	result[i].low = ins2[i2].low;
      vint h;
      if (ins1[i1].high > ins2[i2].high) {
	h = ins1[i1].high;
	i1++; i2++;
	goto ahead1;
      } else if (ins1[i1].high < ins2[i2].high) {
	h = ins2[i2].high;
	i1++; i2++;
	goto ahead2;
      } else {
	result[i].high = ins1[i1].high;
	i1++; i2++; i++;
	if (i1 == max1)
	  goto copy2;
	if (i2 == max2)
	  goto copy1;
	continue;
      }
    ahead1:
      while (i2 < max2 && ins2[i2].high <= h)
	i2++;
      if (i2 == max2) {
	result[i++].high = h;
	goto copy1;
      } else if (ins2[i2].low - 1 <= h) {
	h = ins2[i2++].high;
	goto ahead2;
      } else {
	result[i++].high = h;
	if (i1 == max1)
	  goto copy2;
	continue;
      }
    ahead2:
      while (i1 < max1 && ins1[i1].high <= h)
	i1++;
      if (i1 == max1) {
	result[i++].high = h;
	goto copy2;
      } else if (ins1[i1].low - 1 <= h) {
	h = ins1[i1++].high;
	goto ahead1;
      } else {
	result[i++].high = h;
	if (i2 == max2)
	  goto copy1;
	continue;
      }
    }
  }
 copy1:
  while (i1 < max1)
    result[i++] = ins1[i1++];
  goto done;
 copy2:
  while (i2 < max2)
    result[i++] = ins2[i2++];
 done:
  interval_table_to = ins1;
  this->interval_table = interval_table_from = result;
  this->interval_count = i;
}

/* Find a number in an interval set */
vint Intervals_find(Intervals *this, vint id) {
  vint min = 0;
  vint max = this->interval_count - 1;
  while (min <= max) {
    vint index = (max + min)/2;
    Interval *elem = &(this->interval_table[index]);
    if (id < elem->low)
      max = index - 1;
    else if (id > elem->high)
      min = index + 1;
    else {
      return 1;      
    }
  }
  return 0;
}

/* This function is needed because of the storage method used */
void Intervals_completed(Intervals *this) {
  Assert(this->interval_table == interval_table_from);
  Interval *ins = NEWN(Interval, this->interval_count);
  /* for (vint i = 0; i < this->interval_count; i++) */
  /*   ins[i] = this->interval_table[i]; */
  memcpy(ins, this->interval_table, sizeof(Interval)*this->interval_count);
  this->interval_table = ins;
}

/* ==== SCC: Strong component ==== */

SCC *SCC_new(vint scc_id, vint root_vertex_id, vint *vertex_table) {
  SCC *this = NEW(SCC);
  this->scc_id = scc_id;
  this->root_vertex_id = root_vertex_id;
  this->vertex_count = 0;
  this->vertex_table = vertex_table;
  this->successors = 0;
  return this;
}

vint SCC_successor_scc_count(SCC *this) {
  /* This could be a variable; thus, only a constant cost */
  vint sum = 0;
  Intervals* intervals = this->successors;
  vint i;
  for (i = 0; i < intervals->interval_count; i++)
    sum += intervals->interval_table[i].high - intervals->interval_table[i].low + 1;
  return sum;
}

vint SCC_successor_vertex_count(TC* tc, vint scc_id) {
  /* This could be a variable; thus, only a constant cost */
  vint sum = 0;
  /* MSG("ENTER SCC_successor_vertex_count of SCC " VFMT "\n", scc_id); */
  /* print_scc_struct(scc_id); */
  Intervals *succ = tc->scc_table[scc_id]->successors;
  /* MSG("successors " VFMT "\n", (vint)succ); */
  if (succ != NULL) {
    vint i, j;
    for (i = 0; i < succ->interval_count; i++) {
      Interval *interval = &(succ->interval_table[i]);
      for (j = interval->low; j <= interval->high; j++) {
	sum += tc->scc_table[j]->vertex_count;
      }
    }
  }
  /* MSG("EXIT SCC_successor_vertex_count of SCC " VFMT " is " VFMT "\n", scc_id, sum); */
  return sum;
}

/* ==== TC: the result transitive closure ==== */

TC *TC_new(Digraph *g)
{
  TC *this = NEW(TC);
  vint vertex_count = this->vertex_count = g->vertex_count;
  this->vertex_id_to_scc_id_table = new_vint_table(vertex_count, -1);
  this->scc_table = NEWN(SCC*,vertex_count);
  this->scc_count = 0;
  this->vertex_table = new_vint_table(vertex_count, -1);
  this->vertex_count = this->saved_vertex_count = 0;
  Intervals_initialize_tc(vertex_count);
  return this;
}

SCC *TC_create_scc(TC *this, vint root_id) {
  MSG("create_scc, root=" VFMT "\n", root_id);
  SCC *result = this->scc_table[this->scc_count] = SCC_new(this->scc_count, root_id, this->vertex_table+this->vertex_count);
  this->scc_count++;
  return result;
}

void TC_insert_vertex(TC *this, vint vertex_id) {
    this->vertex_table[this->vertex_count++] = vertex_id;
    this->vertex_id_to_scc_id_table[vertex_id] = this->scc_count-1;
}

void TC_scc_completed(TC *this) {
  this->scc_table[this->scc_count-1]->vertex_count = this->vertex_count - this->saved_vertex_count;
  this->saved_vertex_count = this->vertex_count;
}

SCC *TC_scc_id_to_scc(TC *this, vint scc_id) {
  return this->scc_table[scc_id];
}

Intervals *TC_scc_id_to_successor_set(TC *this, vint scc_id) {
  return this->scc_table[scc_id]->successors;
}

vint TC_vertex_id_to_scc_id(TC *this, vint vertex_id) {
  return this->vertex_id_to_scc_id_table[vertex_id];
}

SCC *TC_vertex_id_to_scc(TC *this, vint vertex_id) {
  return this->scc_table[this->vertex_id_to_scc_id_table[vertex_id]];
}

Intervals *TC_vertex_id_to_successor_set(TC *this, vint vertex_id) {
  return this->scc_table[this->vertex_id_to_scc_id_table[vertex_id]]->successors;
}

vint TC_sccs_edge_exists(TC *this, vint scc_from_id, vint scc_to_id) {
  return Intervals_find(TC_scc_id_to_successor_set(this, scc_from_id), scc_to_id);
}

vint TC_vertices_edge_exists(TC *this, vint vertex_from_id, vint vertex_to_id) {
  return Intervals_find(TC_vertex_id_to_successor_set(this, vertex_from_id),
			   TC_vertex_id_to_scc_id(this, vertex_to_id));
}

/* ==== The algorithm ====
   If the vertex has already been visited, do nothing. Otherwise recursively
   detect the strong component containing the vertex and compute its transitive
   closure. See section 3.4 of the thesis, for a description of algorithm */
static vint visit(vint vertex_id) {
  vint dfn, lowest;
  vint self_loop_p = 0;
  vint *scc_stack_position = scc_stack_top;
  vint edge;
  Vertex *vertex = vertex_table + vertex_id;
  MSG("ENTER visit(" VFMT "), depth_first_numbers[" VFMT "] = " VFMT "\n", vertex_id, vertex_id, depth_first_numbers[vertex_id]);
  if (depth_first_numbers[vertex_id] >= 0) {
    MSG("Already visited, ignore\n");
  } else {
    MSG("Processing ");
    if (SHOW_MSGS) print_vertex_struct(vertex_id);
    MSG("PUSH VERTEX " VFMT " to vertex_stack[" VFMT "]\n", vertex_id, vertex_stack_top - vertex_stack);
    *(vertex_stack_top++) = vertex_id;
    if (SHOW_MSGS) print_vertex_stack();
    depth_first_numbers[vertex_id] = dfn = lowest = depth_first_number_counter++;
    MSG("Set depth_first_numbers[" VFMT "] = " VFMT "\n", vertex_id, dfn);
    for (edge = 0; edge != vertex->outdegree; edge++) {
      vint child = vertex->children[edge];
      vint child_value = depth_first_numbers[child];
      MSG("Processing child[" VFMT "]=" VFMT ", dfn[" VFMT "] = " VFMT ", lowest[" VFMT "] = " VFMT ", child_value = " VFMT "\n",
	      vertex_id, edge, vertex_id, dfn, vertex_id, lowest, depth_first_numbers[child]);
      if (child_value < 0) {
	MSG("Tree edge (" VFMT ", " VFMT "), visit(" VFMT ")\n", vertex_id, child, child);
	child_value = visit(child);
	if (child_value < lowest) {
	  MSG("Visit (" VFMT ") returned new lowest " VFMT "\n", child, child_value);
	  lowest = child_value;
	} else {
	  MSG("Visit (" VFMT ") returned " VFMT "\n", child, child_value);
	}
      } else if (child_value > dfn) {
	MSG("Forward edge (" VFMT ", " VFMT "), ignore\n", vertex_id, child);
      } else {
	vint child_scc_id = vertex_id_to_scc_id_table[child];
	if (child_scc_id >= 0) {
	  MSG("Intercomponent cross edge (" VFMT "," VFMT ")\npush " VFMT " to scc_stack[" VFMT "]\n", vertex_id, child, child_scc_id, scc_stack_top-scc_stack);
	  *(scc_stack_top++) = child_scc_id;
	} else if (child_value < lowest) {
	  MSG("Back edge or intracomponent cross edge (" VFMT "," VFMT ")\nlowest = " VFMT "\n", vertex_id, child, child_value);
	  lowest = child_value;
	} else if (child == vertex_id) {
	  MSG("Self loop edge (" VFMT "," VFMT ")\n", vertex_id, vertex_id);
	  self_loop_p = 1;
	}
      }
    }
    MSG("All children of " VFMT " processed, lowest = " VFMT ", dfn = " VFMT "\n", vertex_id, lowest, dfn);
    if (lowest == dfn) {
      MSG("Vertex " VFMT " is the component root\n", vertex_id);
      SCC *new_scc = TC_create_scc(tc, vertex_id);
      vint scc_id = new_scc->scc_id;
      MSG("generate new component " VFMT ", root = " VFMT "\n", scc_id, vertex_id);
      vint self_insert = self_loop_p || (*(vertex_stack_top-1) != vertex_id);
      MSG("self_insert = " VFMT ", self_loop = " VFMT "\n", self_insert, self_loop_p);
      Intervals *succ = 0;
      vint component_count = scc_stack_top - scc_stack_position;
      MSG("scc_stack contains " VFMT " adjacent components of " VFMT "\n", component_count, scc_id);
      if (self_insert || component_count) {
	MSG("Creating successor set for component " VFMT "\n", scc_id);
	succ = new_scc->successors = Intervals_new();
      }
      if (component_count) {
	vint prev_scc_id = -1;
	MSG("Sort adjacent components\n");
	qsort(scc_stack_position, component_count, sizeof(vint), &cmp_vint);
	MSG("Scanning adjacent components of " VFMT " on scc_stack\n", scc_id);
	while (scc_stack_top != scc_stack_position) {
	  vint scc_id = *(--scc_stack_top);
	  MSG("Popping adjacent component " VFMT " from scc_stack[" VFMT "]\n", scc_id, scc_stack_top-scc_stack);
	  if (scc_id != prev_scc_id) {
	    if (!(Intervals_insert(succ, scc_id))) {
	      MSG("Component " VFMT " not in Succ[" VFMT "], unioning with Succ[" VFMT "]\n", scc_id, scc_id, scc_id);
	      Intervals_union(succ, scc_table[scc_id]->successors);
	    } else {
	      MSG("Component " VFMT " already in Succ[" VFMT "]\n", scc_id, scc_id);
	    }
	    prev_scc_id = scc_id;
	  } else {
	    MSG("Ignoring duplicate " VFMT " in scc_stack[" VFMT "]\n", scc_id, scc_stack_top-scc_stack);
	  }
	}
	MSG("All adjacent components of " VFMT " processed\n", scc_id);
      }
      if (self_insert) {
	Intervals_insert(succ, scc_id);
	MSG("Inserting " VFMT " to its own successor set\n", scc_id);
      }
      if (succ) {
	Intervals_completed(succ);
      }
      MSG("Before vertex_stack while loop:\n");
      if (SHOW_MSGS) print_vertex_stack();
      vint popped_vertex_id;
      do {
	popped_vertex_id = *(--vertex_stack_top);
	MSG("    POP VERTEX " VFMT " from vertex_stack[" VFMT "]\n", popped_vertex_id, vertex_stack_top-vertex_stack);
	TC_insert_vertex(tc, popped_vertex_id);
      } while (popped_vertex_id != vertex_id);
      MSG("After vertex_stack while loop:\n");
      if (SHOW_MSGS) print_vertex_stack();
      TC_scc_completed(tc);
      *(scc_stack_top++) = scc_id;
    }
  }
  MSG("EXIT visit(" VFMT ") returns " VFMT "\n", vertex_id, lowest);
  return lowest;
}

TC* stacktc (Digraph *g)
{
  vint vertex_count = g->vertex_count;
  MSG("stacktc\n");
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

/* ==== Reading the input graph ==== */

typedef struct edge_struct {
  vint from;
  vint to;
} EDGE;

int edge_cmp(const void* a1, const void* a2) {
  EDGE *e1 = (EDGE*)a1;
  EDGE *e2 = (EDGE*)a2;
  if (e1->from < e2->from) return -1;
  else if (e1->from > e2->from) return 1;
  else if (e1->to < e2->from) return -1;
  else if (e1->to > e2->from) return 1;
  else return 0;
}

Digraph *Digraph_read(FILE *input) {
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
  MSG("input: %ld\n", ftell(input));
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
      MSG("'%s', '%s'\n", field1, field2);
    } else {
      fprintf(stderr, "Could not read first line\n");
      return NULL;
    }
  }
  fgetpos(input, &second_line_start);
  edge_count = 0;
  fprintf(stderr, "format = %s\n", VFMT "," VFMT "\n");
  while ((got = fscanf(input, VFMT "," VFMT "\n", &from_vertex, &to_vertex)) == 2) {
    if (from_vertex < 0 || to_vertex < 0) {
      fprintf(stderr, "Illegal edge " VFMT "," VFMT "!\n", from_vertex, to_vertex);
      return NULL;
    }
    edge_count++;
  }
  if (!feof(input)) {
    fprintf(stderr, VFMT " edges read, not at end of file, last got %d!\n", edge_count, got);
    return NULL;
  }
  MSG("Reading " VFMT " edges\n", edge_count);
  edges = NEWN(EDGE, edge_count);
  fsetpos(input, &second_line_start);
  edge_index = 0;
  max_vertex = 0;
  while (fscanf(input, VFMT "," VFMT "\n", &(edges[edge_index].from), &(edges[edge_index].to)) == 2) {
    if (edges[edge_index].from > max_vertex) {
      max_vertex = edges[edge_index].from;
    }
    if (edges[edge_index].to > max_vertex) {
      max_vertex = edges[edge_index].to;
    }
    edge_index++;
  }
  if (edge_index != edge_count) {
    fprintf(stderr, "Something wrong " VFMT " == edge_index != edge_countedge_index != " VFMT "\n", edge_index, edge_count);
  }
  MSG("Sorting edges\n");
  qsort((void*)edges, edge_count, sizeof(EDGE), &edge_cmp);
  vertex_count = max_vertex + 1;
  MSG("Creating " VFMT " vertex_table\n", vertex_count);
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
    MSG("Vertex " VFMT ", " VFMT " edges\n", vi, vertex_table[vi].outdegree);
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
  /* MSG("tc_to_digraph " VFMT " vertices " VFMT " components\n", vertex_count, scc_count); */
  result->vertex_count = vertex_count;
  result->vertex_table = NEWN(Vertex, vertex_count);
  for (i = 0; i < scc_count; i++) {
    edge_count += tc->scc_table[i]->vertex_count*SCC_successor_vertex_count(tc, i);
  }
  /* MSG(VFMT " edges in tc\n", edge_count); */
  edges = new_vint_table(edge_count, -1);
  result->edge_count = edge_count;
  edge_index = 0;
  for (i = 0; i < scc_count; i++) {
    SCC *scc_from = tc->scc_table[i];
    Intervals *succ = scc_from->successors;
    vint to_table_index = 0;
    if (succ != NULL) {
      /* MSG("SCC " VFMT ", " VFMT " intervals\n", i, succ->interval_count); */
      for (j = 0; j < succ->interval_count; j++) {
	Interval *iv = &(succ->interval_table[j]);
	for (l = iv->low; l <= iv->high; l++) {
	  /* MSG("interval " VFMT ".." VFMT ", scc " VFMT "\n", iv->low, iv->high, l); */
	  SCC *scc_to = tc->scc_table[l];
	  for (m = 0; m < scc_to->vertex_count; m++) {
	    /* MSG("copying edge to vertex " VFMT "\n", scc_to->vertex_table[m]); */
	    to_table[to_table_index++] = scc_to->vertex_table[m];
	  }
	}
      }
    }
    qsort(to_table, to_table_index, sizeof(vint), &cmp_vint);
    for (j = 0; j < scc_from->vertex_count; j++) {
      /* MSG("copying to edges[" VFMT ".." VFMT "]\n", edge_index, edge_index + to_table_index-1); */
      memcpy(edges+edge_index, to_table, to_table_index*sizeof(vint));
      k = scc_from->vertex_table[j];
      result->vertex_table[k].vertex_id = k;
      result->vertex_table[k].outdegree = to_table_index;
      result->vertex_table[k].children = edges+edge_index;
      edge_index += to_table_index;
    }
  }
  /* MSG("returning digraph(" VFMT "," VFMT ")\n", result->vertex_count, result->edge_count); */
  return result;
}

/* ==== Converting a digraph into a matrix ==== */

Matrix *Digraph_to_matrix(Digraph *this) {
  vint n = this->vertex_count;
  Matrix *matrix = NEW(Matrix);
  matrix->n = n;
  /* MSG("digraph_to_matrix (" VFMT ", " VFMT ")\n", n, this->edge_count); */
  vint *elements = matrix->elements = (vint*)calloc(n*n, sizeof(vint));
  vint i, j;
  for (i = 0; i < n; i++) {
    Vertex* v = &(this->vertex_table[i]);
    for (j = 0; j < v->outdegree; j++) {
      /* MSG("edge " VFMT "->" VFMT "\n", v->vertex_id, v->children[j]); */
      elements[v->vertex_id*n + v->children[j]] = 1;
    }
  }
  return matrix;
}

/* === Computing the transitive closure of a matrix using Warshall's algorithm ==== */
/* This is much slower than the algorithm stacktc, but it is used for checking the result */
Matrix *warshall(Matrix *this) {
  vint i, j, k;
  vint n = this->n;
  Matrix *result = NEW(Matrix);
  vint *elements = NEWN(vint, n*n);
  memcpy(elements, this->elements, sizeof(vint)*n*n);
  for (k = 0; k < n; k++) {
    for (i = 0; i < n; i++) {
      if (i != k && elements[i*n + k]) {
	for (j = 0; j < n; j++) {
	  if (elements[k*n + j]) {
	    elements[i*n + j] = 1;
	  }
	}
      }
    }
  }
  result->n = n;
  result->elements = elements;
  return result;
}

/* ==== Printing the matrix ==== */
void print_matrix(Matrix *matrix, FILE *output) {
  vint i, j;
  vint *elements = matrix->elements;
  vint n = matrix->n;
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      fprintf(output, "" VFMT "", elements[i*n + j]);
    }
    fprintf(output, "\n");
  }
}

void usage(char* pgm) {
  fprintf(stderr, "usage: %s options input [output]\n", pgm);
  fprintf(stderr, "Options:");
  fprintf(stderr, "    -v | --vertices        Output the vertices and their successor sets (of vertices).\n");
  fprintf(stderr, "    -e | --edges           Output the edges as FROM_VERTEX, TO_VERTEX\n");
  fprintf(stderr, "    -c | --components      Output the components and their successors sets (of components)\n");
  fprintf(stderr, "    -E | --component-edges Output the edges as FROM_COMPONENT, TO_COMPONENT\n");
  fprintf(stderr, "    -i | --intervals       Output the components and their successor sets as intervals. This is the most compact format and the default.\n");
  fprintf(stderr, "    -n | --nothing         Don't output the result\n");
  fprintf(stderr, "    -h | --help            Print help text\n");
  exit(1);
}

enum output_format {
  output_vertices = 1,
  output_edges = 2,
  output_components = 3,
  output_component_edges = 4,
  output_intervals = 5,
  output_nothing = 6
};

void output_tc_vertices(TC* tc, FILE* output, enum output_format output_as) {
  vint* vertices = tc->vertex_table;
  vint n = tc->vertex_count;
  vint* vertex_to_scc = tc->vertex_id_to_scc_id_table;
  SCC** scc_table = tc->scc_table;
  fprintf(output, "[\n");
  for (vint v = 0; v < n; v++) {
    fprintf(output, "    {\n");
    fprintf(output, "        \"id\": " VFMT ",\n", v);
    fprintf(output, "        \"successors\": [");
    SCC* from_scc = scc_table[vertex_to_scc[v]];
    Intervals* intervals = from_scc->successors;
    if (intervals != NULL) {
      char* sep = "";
      for (vint i = 0; i < intervals->interval_count; i++) {
	Interval *interval = &(intervals->interval_table[i]);
	for (vint t = interval->low; t <= interval->high; t++) {
	  SCC* to_scc = tc->scc_table[t];
	  for (vint w = 0; w < to_scc->vertex_count; w++) {
	    fprintf(output, "%s" VFMT, sep, to_scc->vertex_table[w]);
	    sep = ", ";
	  }
	}
      }
    }
    fprintf(output, "]\n");
    if (v < n - 1) {
      fprintf(output, "    },\n");
    } else {
      fprintf(output, "    }\n");
    }
  }
  fprintf(output, "]\n");
}

void output_tc_components(TC* tc, FILE* output, enum output_format output_as) {
  SCC** components = tc->scc_table;
  vint scc_count = tc->scc_count;
  fprintf(output, "[\n");
  for (vint i = 0; i < scc_count; i++) {
    SCC *scc = components[i];
    fprintf(output, "    {\n");
    fprintf(output, "        \"scc\": " VFMT ",\n", i);
    fprintf(output, "        \"root\": " VFMT ",\n", scc->root_vertex_id);
    fprintf(output, "        \"vertices\": [");
    char* sep = "";
    for (vint j = 0; j < scc->vertex_count; j++) {
      fprintf(output, "%s" VFMT, sep, scc->vertex_table[j]);
      sep = ", ";
    }
    fprintf(output, "],\n");
    char* succ_tag = (output_as == output_intervals ? "intervals" : "successors");
    fprintf(output, "        \"%s\": [", succ_tag);
    Intervals* intervals = scc->successors;
    if (intervals != NULL) {
      char* sep = "";
      for (vint j = 0; j < intervals->interval_count; j++) {
	Interval *interval = &(intervals->interval_table[j]);
	if (output_as == output_intervals) {
	  fprintf(output, "%s{\"low\": " VFMT ", \"high\": " VFMT "}", sep, interval->low, interval->high);
	  sep = ", ";
	} else {
	  for (vint c = interval->low; c <= interval->high; c++) {
	    fprintf(output, "%s" VFMT, sep, c);
	    sep = ", ";
	  }
	}
      }
    }
    fprintf(output, "]\n");
    if (i < scc_count - 1) {
      fprintf(output, "    },\n");
    } else {
      fprintf(output, "    }\n");
    }
  }
  fprintf(output, "]\n");
}

void output_tc_edges(TC* tc, FILE* output, enum output_format output_as) {
  SCC** scc_table = tc->scc_table;
  vint scc_count = tc->scc_count;
  for (vint i1 = 0; i1 < scc_count; i1++) {
    SCC *from_scc = scc_table[i1];
    for (vint j1 = 0; j1 < from_scc->vertex_count; j1++) {
      vint from_vertex_id = from_scc->vertex_table[j1];
      Intervals* intervals = from_scc->successors;
      if (intervals != NULL) {
	for (vint k = 0; k < intervals->interval_count; k++) {
	  Interval *interval = &(intervals->interval_table[k]);
	  for (vint i2 = interval->low; i2 <= interval->high; i2++) {
	    SCC *to_scc = scc_table[i2];
	    for (vint j2 = 0; j2 < to_scc->vertex_count; j2++) {
	      vint to_vertex_id = to_scc->vertex_table[j2];
	      fprintf(output, VFMT "," VFMT "\n", from_vertex_id, to_vertex_id);
	    }
	  }
	}
      }
    }
  }
}

void output_tc_component_edges(TC* tc, FILE* output, enum output_format output_as) {
  SCC** components = tc->scc_table;
  vint scc_count = tc->scc_count;
  for (vint i = 0; i < scc_count; i++) {
    SCC *scc = components[i];
    vint from_id = scc->scc_id;
    Intervals* intervals = scc->successors;
    if (intervals != NULL) {
      for (vint j = 0; j < intervals->interval_count; j++) {
	Interval *interval = &(intervals->interval_table[j]);
	for (vint to_id = interval->low; to_id <= interval->high; to_id++) {
	  fprintf(output, VFMT "," VFMT "\n", from_id, to_id);
	}
      }
    }
  }
}

void output_result(TC* result, FILE* output, enum output_format output_as) {
  switch (output_as) {
  case output_vertices:
    output_tc_vertices(result, output, output_as);
    break;
  case output_edges:
    output_tc_edges(result, output, output_as);
    break;
  case output_components:
  case output_intervals:
    output_tc_components(result, output, output_as);
    break;
  case output_component_edges:
    output_tc_component_edges(result, output, output_as);
  case output_nothing:
    break;
  }
}

/* ==== Main program ==== */
int main(int argc, char** argv) {
  Digraph *input_graph,*output_graph;
  TC *stack_tc_result;
  Matrix *m;
  char* pgm = argv[0];
  for (int i = 0; i < argc; i++) {
    fprintf(stderr, "argv[%d] = %s\n", i, argv[i]);
  }
  enum output_format output_tc_as = output_intervals;
  int i = 1;
  int compare_with_warshall = 0;
  for (; i < argc; i++) {
    char *arg = argv[i];
    fprintf(stderr, "arg = %s\n", arg);
    if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
      usage(pgm);
    } else if (!strcmp(arg, "-v") || !strcmp(arg, "--vertices")) {
      output_tc_as = output_vertices;
    } else if (!strcmp(arg, "-e") || !strcmp(arg, "--edges")) {
      output_tc_as = output_edges;
    } else if (!strcmp(arg, "-c") || !strcmp(arg, "--components")) {
      output_tc_as = output_components;
    } else if (!strcmp(arg, "-E") || !strcmp(arg, "--component-edges")) {
      output_tc_as = output_component_edges;
    } else if (!strcmp(arg, "-i") || !strcmp(arg, "--intervals")) {
      output_tc_as = output_intervals;
    } else if (!strcmp(arg, "-n") || !strcmp(arg, "--nothing")) {
      output_tc_as = output_nothing;
    } else if (!strcmp(arg, "-w") || !strcmp(arg, "--warshall")) {
      compare_with_warshall = 1;
    } else if (strlen(arg) > 0 && arg[0] == '-') {
      fprintf(stderr, "%s: Unknown option %s\n", pgm, arg);
      exit(1);
    } else {
      break;
    }
  }
  if (argc - i < 1 || argc - i > 2) {
    usage(pgm);
  }
  char* input_file = argv[i];
  FILE *input;
  if (!(input = fopen(input_file, "r"))) {
    fprintf(stderr, "%s: Cannot open input file %s\n", pgm, input_file);
    exit(1);
  }
  input_graph = Digraph_read(input);
  fclose(input);
  MSG("Stacktc\n");
  stack_tc_result = stacktc(input_graph);

  if (compare_with_warshall) {
    Matrix *input_matrix = Digraph_to_matrix(input_graph);
    MSG("Input graph as matrix\n");
    if (SHOW_MSGS) print_matrix(m, stderr);
    Matrix *warshall_matrix = warshall(input_matrix);
    MSG("Transitive closure by Warshall's algorithm\n");
    if (SHOW_MSGS) print_matrix(warshall_matrix, stderr);
    output_graph = tc_to_digraph(stack_tc_result);
    Matrix *stack_tc_matrix = Digraph_to_matrix(output_graph);
    MSG("Stacktc result as matrix\n");
    if (SHOW_MSGS) print_matrix(stack_tc_matrix, stderr);
    Assert(stack_tc_matrix->n == warshall_matrix->n);
    int n = warshall_matrix->n;
    vint *warshall_elements = warshall_matrix->elements;
    vint *stack_tc_elements = stack_tc_matrix->elements;
    int same = 1;
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
	if (stack_tc_elements[i*n + j] != warshall_elements[i*n + j]) {
	  fprintf(stderr, "difference at (%d,%d)\n", i, j);
	  same = 0;
	}
      }
    }
    if (!same) {
      fprintf(stderr, "Stack_tc and Warshall results are not equal!");
    }
  }
  fprintf(stderr, "output_as %d\n", output_tc_as);
  FILE* output = stdout;
  if (argc - i == 2) {
    char* output_file = argv[i + 1];
    if (!(output = fopen(output_file, "w"))) {
      fprintf(stderr, "%s: Cannot open output file %s\n", pgm, input_file);
      exit(1);
    }
  }
  output_result(stack_tc_result, output, output_tc_as);
  if (output != stdout) {
    fclose(output);
  }
}
