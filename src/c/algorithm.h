/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: algorithm.h

  The algorithm stacktc described in section 3.4 of
  E. Nuutila: Efficient transitive closure computation in large digraphs,
  PhD thesis, Helsinki University of Technology, Laboratory of Information
  Processing Science, 1995.

  https://github.com/eskonuutila/tc/blob/master/docs/thesis.pdf

  =============================================================================
*/

#ifndef _algorithm_h_
#define _algorithm_h_

#include "types.h"
#include "macros.h"
#include "util.h"
#include "intervals.h"
#include "scc.h"
#include "tc.h"
#include "debug.h"

TC* stacktc (Digraph *g);

#endif


