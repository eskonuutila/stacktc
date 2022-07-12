/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: output.h

  Operations for outputting the result transitive closure.
  =============================================================================
*/

#ifndef _output_h_
#define _output_h_

#include "types.h"
#include "macros.h"
#include "util.h"

void output_tc_vertices(TC* tc, FILE* output, enum output_format output_as);

void output_tc_components(TC* tc, FILE* output, enum output_format output_as);

void output_tc_edges(TC* tc, FILE* output, enum output_format output_as);

void output_tc_component_edges(TC* tc, FILE* output, enum output_format output_as);

void output_result(TC* result, char* output_file, enum output_format output_as);

void output_matrix(Matrix *matrix, FILE *output);

#endif
