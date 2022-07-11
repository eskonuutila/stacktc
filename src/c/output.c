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
