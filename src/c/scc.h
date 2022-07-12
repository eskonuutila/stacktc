/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: scc.h

  Strong component operations.
  =============================================================================
*/
#ifndef _scc_h_
#define _scc_h_

#include "types.h"
#include "macros.h"
#include "util.h"

SCC *SCC_new(vint scc_id, vint root_vertex_id, vint *vertex_table);
vint SCC_successor_scc_count(SCC *this);
vint SCC_successor_vertex_count(TC* tc, vint scc_id);

#endif
