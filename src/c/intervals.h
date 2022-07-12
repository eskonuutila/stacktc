/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: intervals.h

  The the interval representation of successor sets described in:
  E. Nuutila: Efficient transitive closure computation in large digraphs,
  PhD thesis, Helsinki University of Technology, Laboratory of Information
  Processing Science, 1995.

  https://github.com/eskonuutila/tc/blob/master/docs/thesis.pdf

  An interval set contains a set of non-overlapping continuous sequences of
  numbers called intervals. An interval is represented by its smallest and
  largest element.

  For example, the set {0, 1, 2, 5, 6, 7, 8, 12} can be represented as
  the intervals set {[0,2], [5,8], [12, 12]}.
   
  As presented in section 4.2 of the thesis, using interval set representation
  makes transitive closure computation much faster and the resulting transitive
  closure can be represented in much smaller space than if we were storing
  individual vertex of strong component numbers in regular sets.

  WARNING! This implementation is optimized just for the stack_tc algorithm.
  For other uses you should change the memory management.
  =============================================================================
*/

#ifndef _intervals_h_
#define _intervals_h_

#include "types.h"
#include "macros.h"
#include "util.h"

void Intervals_initialize_tc(vint max_ids);
Intervals *Intervals_new();
void Intervals_completed(Intervals *this);
vint Intervals_insert(Intervals *this, vint id);
void Intervals_union(Intervals *this, Intervals *other);
vint Intervals_find(Intervals *this, vint id);

#endif
