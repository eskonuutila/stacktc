/*  -*- Mode: C -*-                                                              */
/*-------------------------------------------------------------------------------*/
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
/*-------------------------------------------------------------------------------*/

#include "tc.h"

/*----------------------*/
/* Accessing the result */      

TCSCCIter *TCSCCIter_new(TCSCCIter *this, TC* tc, vint from_scc_id, int reversep) {
  if (this == NULL) {
    this = NEW(TCSCCIter);
  }
  SCC *from_scc = tc->scc_table[from_scc_id];
  this->reversep = reversep;
  this->intervals = from_scc->successors;
  if (reversep) {
    this->current_interval_index = this->intervals->interval_count;
    this->interval_limit = -1;
    this->to_scc_id = this->to_scc_limit = 0;
  } else {
    this->current_interval_index = -1;
    this->interval_limit = this->intervals->interval_count;
    this->to_scc_id = this->to_scc_limit = 0;
  }
  return this;
}

int TCSCCIter_next(TCSCCIter *this) {
  if (this->reversep) {
    if (this->to_scc_id == this->to_scc_limit) {
      if (--this->current_interval_index <= this->interval_limit)
	return ITER_FINISHED;
      else {
	this->to_scc_id = this->intervals->interval_table[this->current_interval_index].high + 1;
	this->to_scc_limit = this->intervals->interval_table[this->current_interval_index].low;
      }
    }
    return --this->to_scc_id;
  } else {
    if (this->to_scc_id == this->to_scc_limit) {
      if (++this->current_interval_index >= this->interval_limit)
	return ITER_FINISHED;
      else {
	this->to_scc_id = this->intervals->interval_table[this->current_interval_index].low - 1;
	this->to_scc_limit = this->intervals->interval_table[this->current_interval_index].high;
      }
    }
    return ++this->to_scc_id;
  }
}

/*------------------------------------------------*/

Interval *interval_table_from = 0;
Interval *interval_table_to = 0;

vint *new_vint_table(vint nelem, vint init) {
  vint *table = NEWN(vint, nelem);
  vint i;
  for (i = 0; i < nelem; i++) {
    table[i] = init;
  }
  return table;
}

int cmp_vint(const void *a, const void *b) {
  return (*((vint*)a) - *((vint*)b));
}

void Intervals_initialize_closure(vint max_ids) {
  interval_table_to = NEWN(Interval, max_ids/2+1);
  interval_table_from = NEWN(Interval, max_ids/2+1);
}

Intervals *Intervals_new() {
  Intervals *this = NEW(Intervals);
  this->interval_count = 0;
  this->interval_table = interval_table_from;
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
  Intervals *succ = tc->scc_table[scc_id]->successors;
  vint i, j;
  for (i = 0; i < succ->interval_count; i++) {
    Interval *interval = &(succ->interval_table[i]);
    for (j = interval->low; j <= interval->high; j++) {
      sum += tc->scc_table[j]->vertex_count;
    }
  }
  return sum;
}

void Intervals_completed(Intervals *this) {
  Assert(this->interval_table == interval_table_from);
  Interval *ins = NEWN(Interval, this->interval_count);
  /* for (vint i = 0; i < this->interval_count; i++) */
  /*   ins[i] = this->interval_table[i]; */
  memcpy(ins, this->interval_table, sizeof(Interval)*this->interval_count);
  this->interval_table = ins;
}

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

SCC *SCC_new(vint scc_id, vint root_vertex_id, vint *vertex_table) {
  SCC *this = NEW(SCC);
  this->scc_id = scc_id;
  this->root_vertex_id = root_vertex_id;
  this->vertex_count = 0;
  this->vertex_table = vertex_table;
  this->successors = 0;
  return this;
}

TC *TC_new(Digraph *g)
{
  TC *this = NEW(TC);
  vint vertex_count = this->vertex_count = g->vertex_count;
  this->vertex_id_to_scc_id_table = new_vint_table(vertex_count, -1);
  this->scc_table = NEWN(SCC*,vertex_count);
  this->scc_count = 0;
  this->vertex_table = new_vint_table(vertex_count, -1);
  this->vertex_count = this->saved_vertex_count = 0;
  Intervals_initialize_closure(vertex_count);
  return this;
}

SCC *TC_create_scc(TC *this, vint root_id) {
  MESSAGE("create_scc, root=" VFMT "\n", root_id);
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

vint *vertex_stack, *vertex_stack_top;
vint *depth_first_numbers;
vint depth_first_number_counter;
Vertex *vertex_table;
TC *tc;
vint *vertex_id_to_scc_id_table;
SCC **scc_table;
vint *scc_stack, *scc_stack_top;

static vint visit(vint vertex_id) {
  vint dfn, lowest;
  vint self_loop_p = 0;
  vint *scc_stack_position = scc_stack_top;
  vint edge;
  Vertex *vertex = vertex_table + vertex_id;
  *(vertex_stack_top++) = vertex_id;
  MESSAGE("ENTER visit(" VFMT "), depth_first_numbers[" VFMT "] = " VFMT "\n", vertex_id, vertex_id, depth_first_numbers[vertex_id]);
  if (depth_first_numbers[vertex_id] >= 0) {
    MESSAGE("Already visited, ignore\n");
  } else {
    depth_first_numbers[vertex_id] = dfn = lowest = depth_first_number_counter++;
    MESSAGE("Set depth_first_numbers[" VFMT "] = " VFMT "\n", vertex_id, dfn);
    for (edge = 0; edge != vertex->outdegree; edge++) {
      vint child = vertex->children[edge];
      vint child_value = depth_first_numbers[child];
      MESSAGE("Processing child[" VFMT "]=" VFMT ", dfn[" VFMT "] = " VFMT ", lowest[" VFMT "] = " VFMT ", child_value = " VFMT "\n",
	      vertex_id, edge, vertex_id, dfn, vertex_id, lowest, depth_first_numbers[child]);
      if (child_value < 0) {
	MESSAGE("Tree edge (" VFMT ", " VFMT "), visit(" VFMT ")\n", vertex_id, child, child);
	child_value = visit(child);
	if (child_value < lowest) {
	  MESSAGE("Visit (" VFMT ") returned new lowest " VFMT "\n", child, child_value);
	  lowest = child_value;
	} else {
	  MESSAGE("Visit (" VFMT ") returned " VFMT "\n", child, child_value);
	}
      } else if (child_value > dfn) {
	MESSAGE("Forward edge (" VFMT ", " VFMT "), ignore\n", vertex_id, child);
      } else {
	vint child_scc_id = vertex_id_to_scc_id_table[child];
	if (child_scc_id >= 0) {
	  MESSAGE("Intercomponent cross edge (" VFMT "," VFMT ")\npush " VFMT " to scc_stack[" VFMT "]\n", vertex_id, child, child_scc_id, scc_stack_top-scc_stack);
	  *(scc_stack_top++) = child_scc_id;
	} else if (child_value < lowest) {
	  MESSAGE("Back edge or intracomponent cross edge (" VFMT "," VFMT ")\nlowest = " VFMT "\n", vertex_id, child, child_value);
	  lowest = child_value;
	} else if (child == vertex_id) {
	  MESSAGE("Self loop edge (" VFMT "," VFMT ")\n", vertex_id, vertex_id);
	  self_loop_p = 1;
	}
      }
      MESSAGE("All children of " VFMT " processed, lowest = " VFMT ", dfn = " VFMT "\n", vertex_id, lowest, dfn);
      if (lowest == dfn) {
	MESSAGE("Vertex " VFMT " is the component root\n", vertex_id);
	SCC *new_scc = TC_create_scc(tc, vertex_id);
	vint scc_id = new_scc->scc_id;
	MESSAGE("generate new component " VFMT ", root = " VFMT "\n", scc_id, vertex_id);
	vint self_insert = self_loop_p || (*(vertex_stack_top-1) != vertex_id);
	MESSAGE("self_insert = " VFMT ", self_loop = " VFMT "\n", self_insert, self_loop_p);
	Intervals *succ = 0;
	vint component_count = scc_stack_top - scc_stack_position;
	MESSAGE("scc_stack contains " VFMT " adjacent components of " VFMT "\n", component_count, scc_id);
	if (self_insert || component_count) {
	  MESSAGE("Creating successor set for component " VFMT "\n", scc_id);
	  succ = new_scc->successors = Intervals_new();
	}
	if (component_count) {
	  vint prev_scc_id = -1;
	  MESSAGE("Sort adjacent components\n");
	  qsort(scc_stack_position, component_count, sizeof(vint), &cmp_vint);
	  MESSAGE("Scanning adjacent components of " VFMT " on scc_stack\n", scc_id);
	  while (scc_stack_top != scc_stack_position) {
	    vint scc_id = *(--scc_stack_top);
	    MESSAGE("Popping adjacent component " VFMT " from scc_stack[" VFMT "]\n", scc_id, scc_stack_top-scc_stack);
	    if (scc_id != prev_scc_id) {
	      if (!(Intervals_insert(succ, scc_id))) {
		MESSAGE("Component " VFMT " not in Succ[" VFMT "], unioning with Succ[" VFMT "]\n", scc_id, scc_id, scc_id);
		Intervals_union(succ, scc_table[scc_id]->successors);
	      } else {
		MESSAGE("Component " VFMT " already in Succ[" VFMT "]\n", scc_id, scc_id);
	      }
	      prev_scc_id = scc_id;
	    } else {
	      MESSAGE("Ignoring duplicate " VFMT " in scc_stack[" VFMT "]\n", scc_id, scc_stack_top-scc_stack);
	    }
	  }
	  MESSAGE("All adjacent components of " VFMT " processed\n", scc_id);
	}
	if (self_insert) {
	  Intervals_insert(succ, scc_id);
	  MESSAGE("Inserting " VFMT " to its own successor set\n", scc_id);
	}
	if (succ) {
	  Intervals_completed(succ);
	}
	MESSAGE("vertex_stack while loop:\n");
	vint popped_vertex_id;
	do {
	  popped_vertex_id = *(--vertex_stack_top);
	  MESSAGE("Popping " VFMT " from vertex_stack[" VFMT "]\n", popped_vertex_id, vertex_stack_top-vertex_stack);
	  TC_insert_vertex(tc, popped_vertex_id);
	} while (popped_vertex_id != vertex_id);
	TC_scc_completed(tc);
	*(scc_stack_top++) = scc_id;
      }
    }
  }
  MESSAGE("EXIT visit(" VFMT ") returns " VFMT "\n", vertex_id, lowest);
  return lowest;
}

TC* stacktc (Digraph *g)
{
  vint vertex_count = g->vertex_count;
  MESSAGE("stacktc\n");
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

/* ************************************** */

/* TC *TC_compact(TC *this) { */
/*   vint scc_count = this->scc_count; */
/*   SCC *compact_scc_table = NEWN(SCC, scc_count); */
/*   memcpy(compact_scc_table, this->scc_table, sizeof(SCC)* */
/* } */

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
  MESSAGE("input: %ld\n", ftell(input));
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
      MESSAGE("'%s', '%s'\n", field1, field2);
    } else {
      fprintf(stderr, "Could not read first line\n");
      return NULL;
    }
  }
  fgetpos(input, &second_line_start);
  edge_count = 0;
  fprintf(stderr, "format = %s\n", VFMT " " VFMT "\n");
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
  MESSAGE("Reading " VFMT " edges\n", edge_count);
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
  MESSAGE("Sorting edges\n");
  qsort((void*)edges, edge_count, sizeof(EDGE), &edge_cmp);
  vertex_count = max_vertex + 1;
  MESSAGE("Creating " VFMT " vertex_table\n", vertex_count);
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
    MESSAGE("Vertex " VFMT ", " VFMT " edges\n", vi, vertex_table[vi].outdegree);
  }
  result = NEW(Digraph);
  result->vertex_count = vertex_count;
  result->edge_count = edge_count;
  result->edge_table = children;
  result->vertex_table = vertex_table;
  return result;
}

Digraph *tc_to_digraph(TC *tc) {
  Digraph *result = NEW(Digraph);
  vint vertex_count = tc->vertex_count;
  vint scc_count = tc->scc_count;
  vint edge_count = 0;
  vint *edges;
  vint i, j, k, l, m;
  vint edge_index;
  vint *to_table = new_vint_table(vertex_count, -1);
  result->vertex_count = vertex_count;
  result->vertex_table = NEWN(Vertex, vertex_count);
  for (i = 0; i < scc_count; i++) {
    edge_count += tc->scc_table[i]->vertex_count*SCC_successor_vertex_count(tc, i);
  }
  MESSAGE(VFMT " edges in tc\n", edge_count);
  edges = new_vint_table(edge_count, -1);
  result->edge_count = edge_count;
  edge_index = 0;
  for (i = 0; i < scc_count; i++) {
    SCC *scc_from = tc->scc_table[i];
    Intervals *succ = scc_from->successors;
    vint to_table_index = 0;
    MESSAGE("SCC " VFMT ", " VFMT " intervals\n", i, succ->interval_count);
    for (j = 0; j < succ->interval_count; j++) {
	Interval *iv = &(succ->interval_table[j]);
	for (l = iv->low; l <= iv->high; l++) {
	  MESSAGE("interval " VFMT ".." VFMT ", scc " VFMT "\n", iv->low, iv->high, l);
	  SCC *scc_to = tc->scc_table[l];
	  for (m = 0; m < scc_to->vertex_count; m++) {
	    MESSAGE("copying edge to vertex " VFMT "\n", scc_to->vertex_table[m]);
	    to_table[to_table_index++] = scc_to->vertex_table[m];
	  }
	}
    }
    qsort(to_table, to_table_index, sizeof(vint), &cmp_vint);
    for (j = 0; j < scc_from->vertex_count; j++) {
      MESSAGE("copying to edges[" VFMT ".." VFMT "]\n", edge_index, edge_index + to_table_index-1);
      memcpy(edges+edge_index, to_table, to_table_index*sizeof(vint));
      k = scc_from->vertex_table[j];
      result->vertex_table[k].vertex_id = k;
      result->vertex_table[k].outdegree = to_table_index;
      result->vertex_table[k].children = edges+edge_index;
      edge_index += to_table_index;
    }
  }
  MESSAGE("returning digraph(" VFMT "," VFMT ")\n", result->vertex_count, result->edge_count);
  return result;
}

Matrix *Digraph_to_matrix(Digraph *this) {
  vint n = this->vertex_count;
  Matrix *matrix = NEW(Matrix);
  matrix->n = n;
  MESSAGE("digraph_to_matrix (" VFMT ", " VFMT ")\n", n, this->edge_count);
  vint *elements = matrix->elements = (vint*)calloc(n*n, sizeof(vint));
  vint i, j;
  for (i = 0; i < n; i++) {
    Vertex* v = &(this->vertex_table[i]);
    for (j = 0; j < v->outdegree; j++) {
      MESSAGE("set " VFMT "->" VFMT "\n", v->vertex_id, v->children[j]);
      elements[v->vertex_id*n + v->children[j]] = 1;
    }
  }
  return matrix;
}

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

int main(int argc, char** argv) {
  FILE *f;
  Digraph *g1,*g2;
  TC *tc1;
  Matrix *m;
  if (argc != 2) {
    printf("usage: %s filename\n", argv[0]);
    exit(1);
  }
  if (!(f = fopen(argv[1], "r"))) {
    printf("%s: Cannot open %s\n", argv[0], argv[1]);
    exit(1);
  }
  g1 = Digraph_read(f);
  fclose(f);
  m = Digraph_to_matrix(g1);
  fprintf(stderr, "Graph\n");
  print_matrix(m, stderr);
  fprintf(stderr, "Transitive closure by Warshall's algorithm\n");
  print_matrix(warshall(m), stderr);
  fprintf(stderr, "Stacktc\n");
  tc1 = stacktc(g1);
  fprintf(stderr, "tc has " VFMT " components\n", tc->scc_count);
  g2 = tc_to_digraph(tc);
  print_matrix(Digraph_to_matrix(g2), stderr);
}
