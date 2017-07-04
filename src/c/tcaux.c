void *interval_set_delete(INTERVAL_SET* this) {
  this->intervals = 0;
  DELETE(this);
}

int interval_set_size(INTERVAL_SET* this) {
  // This could be a variable; thus, only a constant cost
  int sum = this->interval_count;
  int i;
  for (i = 0; i < this->interval_count; i++)
    sum += this->intervals[i].high - this->intervals[i].low;
  return sum;
}

int interval_set_similar(INTERVAL_SET* this, INTERVAL_SET* other) {
  if (!this) return (!other || other->interval_count == 0);
  else if (!other || this->interval_count != other->interval_count) return 0;
  else {
    int i;
    for (i = 0; i < this->interval_count; i++)
      if (this->intervals[i].low != other->intervals[i].low ||
	  this->intervals[i].high != other->intervals[i].high)
	return 0;
    return 1;
  }
}

void interval_set_print(INTERVAL_SET* this, FILE *stream) {
  if (!this) {
    fprintf(stream, "NULL");
    return;
  }
  fprintf(stream, "{");
  int firstp = 1;
  int i;
  for (i = 0; i < this->interval_count; i++)
    int v;
    for (v = this->intervals[i].low; v <= this->intervals[i].high; v++)
      if (firstp) {
	firstp = 0;
	fprintf(stream, "%d", v);
      } else
	fprintf(stream, ", %d", v);
  fprintf(stream, "}");
}

void interval_set_print_structure(INTERVAL_SET* this, FILE *stream, int) {
  if (!this) {
    fprintf(stream, "NULL");
    return;
  }
  fprintf(stream, "{s=%d,c=%d: ", this->size, this->interval_count);
  int firstp = 1;
  int i;
  for (i = 0; i < this->interval_count; i++)
    if (firstp) {
      firstp = 0;
      fprintf(stream, "[%d, %d]", 
	      this->intervals[i].low, this->intervals[i].high);
    } else
      fprintf(stream, ",[%d, %d]", 
	      this->intervals[i].low, this->intervals[i].high);
  fprintf(stream, "}");
}

void interval_set_check(INTERVAL_SET* this) {
  if (this->interval_count > 0) {
    if (this->intervals[0].low > this->intervals[0].high) {
      fprintf(stderr, "Interval corrupted at index 0\n");
      abort();
      return 0;
    }
    int i;
    for (i = 0; i < this->interval_count - 1; i++) {
      if (this->intervals[i+1].low > this->intervals[i+1].high) {
	fprintf(stderr, "Interval corrupted at index %d\n", i);
	abort();
	return 0;
      }
      if (this->intervals[i].high >= this->intervals[i+1].low - 1) {
	fprintf(stderr, "Intervals corrupted at index %d (+1)\n", i);
	abort();
	return 0;
      }
    }
  }
  return 1;
}

void scc_delete(this scc) {
  if (this->successors) DELETE(this->successors);
}

void scc_set_delete(SCC* this_set) {
  DELETE(this->vertex_id_to_scc_id_table);
  for (int i = 0; i <= this->scc_cursor; i++)
    scc_delete(this->scc_table[i]);
  DELETE(this->scc_table);
  DELETE(this->vertex_table);
  DELETE(this->block_allocator);
}

int scc_set_similar(SCC_SET* this, SCC_SET* other, scc_similarity_type stype, FILE* output, int abortp) {
  Assert(this->closure_computed_p && other->closure_computed_p);
  if (scc_count() != other->scc_count()) {
    if (output) fprintf(output, "Different numbers of components: %d, %d\n", scc_count(), other->scc_count());
    RETURN_ZERO_OR_ABORT(abortp);
  }
  for (int c = 0; c < scc_count(); c++) {
    if (scc_id_to_scc(c)->root_vertex_id() != other->scc_id_to_scc(c)->root_vertex_id()) {
      if (output) fprintf(output, "root of scc %d != root of other scc %d\n", c, c);
      RETURN_ZERO_OR_ABORT(abortp);
    }
  }
  if (stype == SCCSameComponentCounts) {
    return 1;
  }
  Assert(this->vertex_cursor == other->vertex_cursor);
  for (i = 0; i < this->vertex_cursor; i++)
    if (scc_set_vertex_id_to_scc_id(this, i) != other->scc_set_vertex_vertex_id_to_scc_id(other,i)) {
      if (output) fprintf(output, "vertex %d in scc %d and scc %d\n",
			  i, scc_set_vertex_id_to_scc_id(this, i), other->scc_set_vertex_vertex_id_to_scc_id(other,i));
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
		   char* vertex_sep=", ", char* component_sep=",\n");

#define RETURN_ZERO_OR_ABORT(x) if (x) abort() else return 0

void scc_set_print_components(SCC_SET* this, FILE *output, char* open, char* close, char* vertex_sep, char* component_sep)
{
  int i, j;
  
  fprintf(output, open);
  int *vertices = 0, *prev_vertices = 0, size = 0, prev_size = 0;
  for (i = 0; i <= this->scc_cursor; i++) {
    SCC* scc = this->scc_table[i];
    if (i > 0) {
      prev_vertices = vertices;
      prev_size = size;
      fprintf(output, component_sep);
    }
    vertices = scc->vertex_table;
    size = scc->size;
    Assert(i == 0 || vertices == prev_vertices + prev_size);
    fprintf(output, open);
    int root_found_p = 0;
    for (j = 0; j < size; j++) {
      if (j == 0)
	fprintf(output, "%d", vertices[j]);
      else
	fprintf(output, "%s%d", vertex_sep, vertices[j]);
      if (vertices[j] == scc->root_vertex_id())
	root_found_p = 1;
    }
    if (!root_found_p)
      printf_abort("Root vertex %d missing from component %d (size %d)\n", scc->root_vertex_id(), scc->scc_id(), scc->size());
    fprintf(output, close);
  }
  fprintf(output, "%s\n", close);
}

void scc_set_print_closure(SCC_SET* this, FILE *output, ClosurePrintStyle pstyle, char* open, char* close, char* vertex_sep, char* component_sep)
{
  int i, j, k;
  
  int flatp;
  if (pstyle == ClosurePrintRanges) {
    pstyle = ClosurePrintStructure;
    flatp = 1;
  } else
    flatp = 0;
  if (pstyle == ClosurePrintFull || pstyle == ClosurePrintEdges) {
    if (pstyle == ClosurePrintEdges)
      // Nasty cludge: n*n edges
      fprintf(output, "%d %d\n", this->graph_vertex_count, this->graph_vertex_count*this->graph_vertex_count);
    else
      fprintf(output, open);
    int *set_vertex_table = new_int_table(this->graph_vertex_count, 0);
    for (i = 0; i < this->vertex_cursor; i++) {
      if (i > 0 && pstyle == ClosurePrintFull) fprintf(output, component_sep);
      int scc_id = scc_set_vertex_vertex_id_to_scc_id(this, i);
      SCC* scc = scc_id_to_scc(scc_id);
      Assert(scc->scc_id() == scc_id);
      INTERVAL_SET* succ = scc->successors();
      Assert(succ || scc->size == 1);
      if (pstyle == ClosurePrintFull) fprintf(output, open);
      if (succ) {
	int vertex_index = 0;
	int si;
	for (si = 0; si < succ->interval_count; si++) {
	  int j = succ->intervals[si];
	  if (!successor_sets_contain_vertices_p()) {
	    SCC* j_scc = scc_id_to_scc(j);
	    Assert(j_scc->scc_id() == j);
	    int root_found_p = 0;
	    int *vertices = j_scc->vertex_table();
	    for (k = 0; k < j_scc->size; k++) {
	      set_vertex_table[vertex_index++] = vertices[k];
	      if (vertices[k] == j_scc->root_vertex_id())
		root_found_p = 1;
	    }
	    if (!root_found_p)
	      printf_abort("Root vertex %d missing from component %d (size %d)\n",
			   j_scc->root_vertex_id(), j_scc->scc_id(), j_scc->size);
	  } else
	    set_vertex_table[vertex_index++] = j;
	}
	qsort(set_vertex_table, vertex_index, sizeof(int), &cmp_int);
	if (pstyle == ClosurePrintFull) {
	  int firstp = 1;
	  for (int ni = 0; ni < vertex_index; ni++) {
	    if (firstp) {
	      firstp = 0;
	      fprintf(output, "%d", set_vertex_table[ni]);
	    } else
	      fprintf(output, "%s%d", vertex_sep, set_vertex_table[ni]);
	  }
	} else {
	  for (int ni = 0; ni < vertex_index; ni++)
	    fprintf(output, "(%d %d)\n", i, set_vertex_table[ni]);
	}
      }
      if (pstyle == ClosurePrintFull) fprintf(output, close);
    }
    DELETE(set_vertex_table);
    if (pstyle == ClosurePrintFull) fprintf(output, "%s\n", close);
  } else {
    fprintf(output, open);
    for (i = 0; i <= this->scc_cursor; i++) {
      SCC* scc = this->scc_table[i];
      Assert(scc->scc_id() == i);
      INTERVAL_SET* succ = scc->successors();
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
	      fprintf(output, "%s%d", vertex_sep, j);
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

void scc_set_check(SCC_SET* this) {
  if (this->closure_computed_p) {
    int *vertices = 0, *prev_vertices = 0, size = 0, prev_size = 0;
    for (int i = 0; i <= this->scc_cursor; i++) {
      SCC* scc = this->scc_table[i];
      Assert(scc->scc_id == i);
      Assert(scc->successors || scc->size == 1);
      if (i > 0) {
	prev_vertices = vertices;
	prev_size = size;
      } else
	Assert(scc->vertex_table == this->vertex_table);
      vertices = scc->vertex_table;
      size = scc->size;
      Assert(i == 0 || vertices == prev_vertices + prev_size);
      int root_found_p = 0;
      for (int j = 0; j < size; j++)
	if (vertices[j] == scc->root_vertex_id)
	  root_found_p = 1;
      Assert(root_found_p);
    }
    Assert(this->vertex_table + this->vertex_cursor == vertices + size);
  }
}

graph* read_graph(FILE* input) {
  int vertex_count;
  int edge_count;
  int vertex_cursor;
  int edge_cursor, saved_edge_cursor = 0;
  int from_vertex, to_vertex, prev_from_vertex;

  if (fscanf(input, "%d %d\n", &vertex_count, &edge_count) != 2) {
    fprintf(stderr, "Illegal input file\n");
    return 0;
  }
  vertex *vertices =  NEW( Vertex[vertex_count]);
  int *edges = new_int_table(edge_count, -1);
  for (vertex_cursor = 0; vertex_cursor < vertex_count; vertex_cursor++) {
    vertices[vertex_cursor].vertex_id = vertex_cursor;
    vertices[vertex_cursor].edge_count = 0;
    vertices[vertex_cursor].children = NULL;
  }
  graph *result;
  prev_from_vertex = -1;
  for (edge_cursor = 0; edge_cursor < edge_count; edge_cursor++) {
    if (fscanf(input, "(%d %d)\n", &from_vertex, &to_vertex) != 2 ||
	from_vertex < 0 || from_vertex >= vertex_count || 
	to_vertex < 0 || to_vertex >= vertex_count) {
      DELETE(vertices);
      DELETE(edges);
      fprintf(stderr, "Illegal input file (edge %d)\n", edge_cursor);
      return 0;
    }
    if (from_vertex != prev_from_vertex) {
      if (prev_from_vertex >= 0)
	vertices[prev_from_vertex].edge_count = edge_cursor - saved_edge_cursor;
      if (vertices[from_vertex].edge_count != 0) {
	DELETE(vertices);
	DELETE(edges);
	fprintf(stderr, "Illegal input file (edge %d out of order)\n", edge_cursor);
	return 0;
      }
      vertices[from_vertex].children = &(edges[edge_cursor]);
      saved_edge_cursor = edge_cursor;
    }
    edges[edge_cursor] = to_vertex;
    prev_from_vertex = from_vertex;
  }
  if (prev_from_vertex >= 0)
    vertices[prev_from_vertex].edge_count = edge_cursor - saved_edge_cursor;
  if (!feof(input))
    fprintf(stderr, "%d edges read, not at end of file!\n", edge_count);
  result = NEW(Graph);
  result->style = GraphCyclic;
  result->method = "Read";
  result->outdegree = (vertex_count > 0 ? ((double)edge_count)/((double)vertex_count) : 0);
  result->vertex_count = vertex_count;
  result->edge_count = edge_cursor;
  result->vertices = vertices;
  result->edges = edges;
  result->components = NULL;
  result->saved_components = NULL;
  return result;
}


