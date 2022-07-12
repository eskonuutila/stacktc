/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: scc.c

  Strong component operations.
  =============================================================================
*/

#include "scc.h"
#include "tc.h"

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
  /* DBG("ENTER SCC_successor_vertex_count of SCC " VFMT "\n", scc_id); */
  /* print_scc_struct(scc_id); */
  Intervals *succ = tc->scc_table[scc_id]->successors;
  /* DBG("successors " VFMT "\n", (vint)succ); */
  if (succ != NULL) {
    vint i, j;
    for (i = 0; i < succ->interval_count; i++) {
      Interval *interval = &(succ->interval_table[i]);
      for (j = interval->low; j <= interval->high; j++) {
	sum += tc->scc_table[j]->vertex_count;
      }
    }
  }
  /* DBG("EXIT SCC_successor_vertex_count of SCC " VFMT " is " VFMT "\n", scc_id, sum); */
  return sum;
}
