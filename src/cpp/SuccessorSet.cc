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
// This file declares types for representing the successor sets.
//
// Ifdefs used directly in this file:
//
//   USE_BIT_SETS: include code for BitSet
//
//   USE_CHAIN_SETS: include code for ChainSet
//
//   USE_INTERVAL_SETS: include code for IntervalSet
//
//   USE_LIST_BIT_SETS: include code for ListBitSet
//
//   USE_LIST_SETS: include code for ListSet
//
//   USE_RANGE_SETS: include code for RangeSet
//

#include "SuccessorSet.h"
#include "Graph.h"

void initialize_successor_sets(
#ifndef USE_NO_SETS
int max_ids
#endif
) {
#if !defined(USE_NO_SETS) && !defined(UNIQUE_SUCCESSOR_SET_TYPE)
  NatSet::initialize(max_ids);
#endif
#ifdef USE_BIT_SETS
  BitSet::initialize(max_ids);
#endif
#ifdef USE_CHAIN_SETS
  ChainSet::initialize(max_ids);
#endif
#ifdef USE_INTERVAL_SETS
  IntervalSet::initialize(max_ids);
#endif
#ifdef USE_LIST_BIT_SETS
  ListBitSet::initialize(max_ids);
#endif
#ifdef USE_LIST_SETS
  ListSet::initialize(max_ids);
#endif
#ifdef USE_RANGE_SETS
  RangeSet::initialize(max_ids);
  RangeTree::initialize(max_ids);
#endif
}

void initialize_free_memories() {
  SCC::initialize_free_memory();
#ifdef USE_BIT_SETS
  BitSet::initialize_free_memory();
  BitSetIter::initialize_free_memory();
#endif
#ifdef USE_CHAIN_SETS
  ChainCell::initialize_free_memory();
  Chains::initialize_free_memory();
  ChainSet::initialize_free_memory();
  ChainSetIter::initialize_free_memory();
#endif
#ifdef USE_INTERVAL_SETS
  IntervalSet::initialize_free_memory();
  IntervalSetIter::initialize_free_memory();
#endif
#ifdef USE_LIST_BIT_SETS
  ListBitSet::initialize_free_memory();
  ListBitSetIter::initialize_free_memory();
#endif
#ifdef USE_LIST_SETS
  ListSet::initialize_free_memory();
  ListSetIter::initialize_free_memory();
#endif
#if defined(USE_LIST_SETS) || defined(USE_LIST_BIT_SETS)
  ListNode::initialize_free_memory();
#endif
#ifdef USE_RANGE_SETS
  RangeSet::initialize_free_memory();
  RangeSetIter::initialize_free_memory();
  RangeTree::initialize_free_memory();
#endif
}
