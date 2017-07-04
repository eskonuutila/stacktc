// -*- Mode: C++ -*-
//
// $RCSfile$
// $Revision$
// $Source$
// $Author$
// $Date$
// $State$
// $Log$
//
// Description:
//
// A set representation that is based on intervals.
//
// Ifdefs:
//  
//   COLLECT_SET_METRICS: if non-zero, inserts code for collecting set metrics.
//   NOTE! You must call set_costing(1) somewhere!
//
//   UNIQUE_SUCCESSOR_SET_TYPE: if non-zero, does not include NatSet superclass
//

#ifndef _interval_set_h
#define _interval_set_h

#include <stdio.h>
#include "config.h"
#include "util.h"
#include "allocs.h"
#include "NatSet.h"
#include "Metric.h"
#include "set-metrics.h"

#ifdef COLLECT_SET_METRICS
const double interval_set_init_cost = 1.0;
const double interval_set_size_cost = 1.0;
const double interval_set_empty_cost = 1.0;
const double interval_set_find_cost = 1.0;
const double interval_set_insert_cost = 1.0;
const double interval_set_unioni_cost = 1.0;
const double interval_set_copy_cost = 1.0;
const double interval_set_iter_init_cost = 1.0;
const double interval_set_iter_next_cost = 1.0;
#endif

SET_METRICS_DECLARE(interval);

class IntervalSetIter;

DECLARE_ALLOC_VARIABLES(IntervalSetIter);

class IntervalSet;

#ifdef UNIQUE_SUCCESSOR_SET_TYPE
class IntervalSetIter {
#else
class IntervalSetIter : public NatSetIter {
#endif
  IntervalSet* v_set;
  int v_clearp;
  int v_reversep;
  int v_index;
  int v_index_limit;
  int v_value;
  int v_value_limit;
  DECLARE_ALLOC_ROUTINES(IntervalSetIter);
 public:
  IntervalSetIter(IntervalSet* set, int clearp=0, int reversep=0);
  int next();
};

DECLARE_ALLOC_VARIABLES(IntervalSet);

struct Interval {
  int low, high;
};

#ifdef UNIQUE_SUCCESSOR_SET_TYPE
class IntervalSet {
#else
class IntervalSet : public NatSet {
#endif
  Interval *v_intervals;
  int v_interval_count;
  DECLARE_ALLOC_ROUTINES(IntervalSet);
  /* s_from is the Interval table of the current set */
  static Interval *s_from = 0;
  /* s_to is used in set operations */
  static Interval *s_to = 0;
 public:
  static BlockAllocator *s_allocator;
  IntervalSet() {
    INSERT_SET_SAMPLE(interval_set_init_metric, interval_set_init_cost);
    v_interval_count = 0;
    Assert(s_from);
    v_intervals = s_from;
  }
  ~IntervalSet() {
    v_intervals = 0;
  }
  void clear() {
    v_intervals = 0;
    v_interval_count = 0;
  }
  int size();
  int empty() {
    INSERT_SET_SAMPLE(interval_set_empty_metric, interval_set_empty_cost);
    return v_interval_count == 0;
  }
  int interval_count() { return v_interval_count; }
  Interval *intervals() { return v_intervals; }
  int find(int id);
  int insert(int id);
//  void insert_interval(int low, int high);
  void unioni(IntervalSet* other);
  void complete();
  void print(FILE* stream=stderr);
  void print_structure(FILE* stream=stderr, int flatp=1);
  int similar(IntervalSet *arg2);
#ifdef UNIQUE_SUCCESSOR_SET_TYPE
  IntervalSetIter* iter(int clearp=0, int reversep=0) { 
#else
  NatSetIter* iter(int clearp=0, int reversep=0) { 
#endif
    return NEW(IntervalSetIter(this, clearp, reversep)); 
  }
#ifdef UNIQUE_SUCCESSOR_SET_TYPE
  void delete_iter(IntervalSetIter *it) {
    DELETE(it);
  }
#else
  void delete_iter(NatSetIter *it) {
    DELETE((IntervalSetIter*)it);
  }
#endif
  int check();
  friend class IntervalSetIter;
  static void initialize(int /* max_ids */) { }
  static void initialize_closure(int /* max_ids */);
};

#endif
