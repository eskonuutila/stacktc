/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: digraph.h

  The implementation of the digraph.
  =============================================================================
*/

#ifndef _digraph_h_
#define _digraph_h_

#include "types.h"
#include "macros.h"
#include "util.h"

Digraph *digraph_read(char *input_file);
Digraph *tc_to_digraph(TC *tc);
Matrix *digraph_to_matrix(Digraph *this);

#endif


