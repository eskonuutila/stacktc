/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: tc.h

  The transitive closure result representation.
  =============================================================================
*/

#ifndef _tc_h_
#define _tc_h_

#include "types.h"
#include "macros.h"
#include "util.h"
#include "intervals.h"
#include "scc.h"

TC *TC_new(Digraph *g);
SCC *TC_create_scc(TC *this, vint root_id);
void TC_insert_vertex(TC *this, vint vertex_id);
void TC_scc_completed(TC *this);
SCC *TC_scc_id_to_scc(TC *this, vint scc_id);
Intervals *TC_scc_id_to_successor_set(TC *this, vint scc_id);
vint TC_vertex_id_to_scc_id(TC *this, vint vertex_id);
SCC *TC_vertex_id_to_scc(TC *this, vint vertex_id);
Intervals *TC_vertex_id_to_successor_set(TC *this, vint vertex_id);
vint TC_sccs_edge_exists(TC *this, vint scc_from_id, vint scc_to_id);
vint TC_vertices_edge_exists(TC *this, vint vertex_from_id, vint vertex_to_id);

#endif
