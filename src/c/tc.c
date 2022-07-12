/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: tc.c

  The transitive closure result representation.
  =============================================================================
*/

#include "tc.h"

TC *TC_new(Digraph *g)
{
  TC *this = NEW(TC);
  vint vertex_count = this->vertex_count = g->vertex_count;
  this->vertex_id_to_scc_id_table = new_vint_table(vertex_count, -1);
  this->scc_table = NEWN(SCC*,vertex_count);
  this->scc_count = 0;
  this->vertex_table = new_vint_table(vertex_count, -1);
  this->vertex_count = this->saved_vertex_count = 0;
  Intervals_initialize_tc(vertex_count);
  return this;
}

SCC *TC_create_scc(TC *this, vint root_id) {
  DBG("create_scc, root=" VFMT "\n", root_id);
  SCC *result = this->scc_table[this->scc_count] = SCC_new(this->scc_count, root_id, this->vertex_table+this->vertex_count);
  this->scc_count++;
  return result;
}

void TC_insert_vertex(TC *this, vint vertex_id) {
    this->vertex_table[this->vertex_count++] = vertex_id;
    this->vertex_id_to_scc_id_table[vertex_id] = this->scc_count-1;
}

void TC_scc_completed(TC *this) {
  this->scc_table[this->scc_count-1]->vertex_count = this->vertex_count - this->saved_vertex_count;
  this->saved_vertex_count = this->vertex_count;
}

SCC *TC_scc_id_to_scc(TC *this, vint scc_id) {
  return this->scc_table[scc_id];
}

Intervals *TC_scc_id_to_successor_set(TC *this, vint scc_id) {
  return this->scc_table[scc_id]->successors;
}

vint TC_vertex_id_to_scc_id(TC *this, vint vertex_id) {
  return this->vertex_id_to_scc_id_table[vertex_id];
}

SCC *TC_vertex_id_to_scc(TC *this, vint vertex_id) {
  return this->scc_table[this->vertex_id_to_scc_id_table[vertex_id]];
}

Intervals *TC_vertex_id_to_successor_set(TC *this, vint vertex_id) {
  return this->scc_table[this->vertex_id_to_scc_id_table[vertex_id]]->successors;
}

vint TC_sccs_edge_exists(TC *this, vint scc_from_id, vint scc_to_id) {
  return Intervals_find(TC_scc_id_to_successor_set(this, scc_from_id), scc_to_id);
}

vint TC_vertices_edge_exists(TC *this, vint vertex_from_id, vint vertex_to_id) {
  return Intervals_find(TC_vertex_id_to_successor_set(this, vertex_from_id),
			   TC_vertex_id_to_scc_id(this, vertex_to_id));
}

