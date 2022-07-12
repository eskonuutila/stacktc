/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: main.c
  =============================================================================
*/

#include "algorithm.h"
#include "warshall.h"
#include "digraph.h"
#include "output.h"

void usage(char* pgm) {
  fprintf(stderr, "usage: %s options [input [output]]\n", pgm);
  fprintf(stderr, "\n");
  fprintf(stderr, "Compute the transitive closure of a digraph using the algorithm stacktc described in\n");
  fprintf(stderr, "E. Nuutila: Efficient transitive closure computation in large digraphs, PhD thesis, \n");
  fprintf(stderr, "Helsinki University of Technology, Laboratory of Information Processing Science, 1995.\n");
  fprintf(stderr, "See https://github.com/eskonuutila/tc/blob/master/docs/thesis.pdf\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "The input is csv file of two columns. The lines are pairs of integers each representing\n");
  fprintf(stderr, "an edge FROM,TO, where FROM and TO are integers representing the vertices of the graph.\n");
  fprintf(stderr, "If the input file (as well as the output file) is omitted or is '-', the input is read\n");
  fprintf(stderr, "from the standard input.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Several output formats are available using the options; the default is '--intervals'.\n");
  fprintf(stderr, "If the output file is omitted or is '-', the output is written to the standard output.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "    -i | --intervals       Output the strong components and their successor sets as intervals.\n");
  fprintf(stderr, "                           This is usually the most compact format. The result is in JSON format.\n");
  fprintf(stderr, "    -c | --components      Output the strong components and their successors sets (of components).\n");
  fprintf(stderr, "                           The result is in JSON format.\n");
  fprintf(stderr, "    -v | --vertices        Output the vertices and their successor sets (of vertices).\n");
  fprintf(stderr, "                           The result is in JSON format.\n");
  fprintf(stderr, "    -E | --component-edges Output as lines of edges FROM_COMPONENT, TO_COMPONENT. The result is in CSV format.\n");
  fprintf(stderr, "    -e | --edges           Output as lines of edges FROM_VERTEX, TO_VERTEX. The result is in CSV format.\n");
  fprintf(stderr, "    -n | --nothing         Don't output the result.\n");
  fprintf(stderr, "    -w | --warshall        Check the result against the matrix based Warshall's algorithm.\n");
  fprintf(stderr, "    -h | --help            Print help text\n");
  exit(1);
}

int main(int argc, char** argv) {
  Digraph *input_graph,*output_graph;
  TC *stack_tc_result;
  Matrix *m;
  char* pgm = argv[0];
  enum output_format output_tc_as = output_intervals;
  int i = 1;
  int compare_with_warshall = 0;
  for (; i < argc; i++) {
    char *arg = argv[i];
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
  input_graph = digraph_read(input_file);

  DBG("Stacktc\n");
  stack_tc_result = stacktc(input_graph);

  if (compare_with_warshall) {
    Matrix *input_matrix = digraph_to_matrix(input_graph);
    DBG("Input graph as matrix\n");
    DBGCALL(output_matrix(m, stderr));
    Matrix *warshall_matrix = warshall(input_matrix);
    DBG("Transitive closure by Warshall's algorithm\n");
    DBGCALL(output_matrix(warshall_matrix, stderr));
    output_graph = tc_to_digraph(stack_tc_result);
    Matrix *stack_tc_matrix = digraph_to_matrix(output_graph);
    DBG("Stacktc result as matrix\n");
    DBGCALL(output_matrix(stack_tc_matrix, stderr));
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
    if (same) {
      fprintf(stderr, "Stacktc and Warshall results are equal.\n");
    } else {
      fprintf(stderr, "Stacktc and Warshall results are not equal!\n");
    }
  }
  output_result(stack_tc_result, (argc - i == 2 ? argv[i + 1] : NULL), output_tc_as);
}
