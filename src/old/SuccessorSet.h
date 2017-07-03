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
// Ifdefs:
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
//   USE_ALL_SETS: all above
//
//   USE_NO_SETS: none above
//
//   DEBUG_ALLOCATIONS: if non-zero, debug information is collected for each
//   allocation.
//

#ifndef _SuccessorSet_h
#define _SuccessorSet_h

#include "config.h"

void initialize_successor_sets(int max_ids);
void initialize_free_memories();

enum SetType { TypeBitSet, TypeChainSet, TypeIntervalSet,
	       TypeListBitSet, TypeListSet, TypeRangeSet };

extern SetType successor_set_type;

#ifndef USE_NO_SETS

#ifndef UNIQUE_SUCCESSOR_SET_TYPE
#include "NatSet.h"
#endif

#ifdef USE_BIT_SETS
#include "BitSet.h"
#define MAYBE_CREATE_BIT_SET(x,r) ((x) == TypeBitSet ? NEW(BitSet) : (r))
#define MAYBE_SIMILAR_BIT_SET(t,x,y,r) \
((t) == TypeBitSet ? \
 ((BitSet*)(x))->similar((BitSet*)(y)) : r)
#define MAYBE_BIT_SET_UNION(x,y)\
    case TypeBitSet:\
      ((BitSet*)(x))->unioni((BitSet*)(y));\
      break;
#else
#define MAYBE_CREATE_BIT_SET(x,r) (r)
#define MAYBE_SIMILAR_BIT_SET(t,x,y,r) (r)
#define MAYBE_BIT_SET_UNION(x,y)
#endif

#ifdef USE_CHAIN_SETS
#include "ChainSet.h"
#define MAYBE_CREATE_CHAIN_SET(x,r) ((x) == TypeChainSet ? NEW(ChainSet(Chains::s_current)) : (r))
#define MAYBE_SIMILAR_CHAIN_SET(t,x,y,r) \
((t) == TypeChainSet ? \
 ((ChainSet*)(x))->similar((ChainSet*)(y)) : r)
#define MAYBE_CHAIN_SET_UNION(x,y)\
    case TypeChainSet:\
      ((ChainSet*)(x))->unioni((ChainSet*)(y));\
      break;
#else
#define MAYBE_CREATE_CHAIN_SET(x,r) (r)
#define MAYBE_SIMILAR_CHAIN_SET(t,x,y,r) (r)
#define MAYBE_CHAIN_SET_UNION(x,y)
#endif

#ifdef USE_INTERVAL_SETS
#include "IntervalSet.h"
#define MAYBE_CREATE_INTERVAL_SET(x,r) ((x) == TypeIntervalSet ? NEW(IntervalSet) : (r))
#define MAYBE_SIMILAR_INTERVAL_SET(t,x,y,r) \
((t) == TypeIntervalSet ? \
 ((IntervalSet*)(x))->similar((IntervalSet*)(y)) : r)
#define MAYBE_INTERVAL_SET_UNION(x,y)\
    case TypeIntervalSet:\
      ((IntervalSet*)(x))->unioni((IntervalSet*)(y));\
      break;
#else
#define MAYBE_CREATE_INTERVAL_SET(x,r) (r)
#define MAYBE_SIMILAR_INTERVAL_SET(t,x,y,r) (r)
#define MAYBE_INTERVAL_SET_UNION(x,y)
#endif

#ifdef USE_LIST_BIT_SETS
#include "ListBitSet.h"
#define MAYBE_CREATE_LIST_BIT_SET(x,r) ((x) == TypeListBitSet ? NEW(ListBitSet) : (r))
#define MAYBE_SIMILAR_LIST_BIT_SET(t,x,y,r) \
((t) == TypeListBitSet ? \
 ((ListBitSet*)(x))->similar((ListBitSet*)(y)) : r)
#define MAYBE_LIST_BIT_SET_UNION(x,y)\
    case TypeListBitSet:\
      ((ListBitSet*)(x))->unioni((ListBitSet*)(y));\
      break;
#else
#define MAYBE_CREATE_LIST_BIT_SET(x,r) (r)
#define MAYBE_SIMILAR_LIST_BIT_SET(t,x,y,r) (r)
#define MAYBE_LIST_BIT_SET_UNION(x,y)
#endif

#ifdef USE_LIST_SETS
#include "ListSet.h"
#define MAYBE_CREATE_LIST_SET(x,r) ((x) == TypeListSet ? NEW(ListSet) : (r))
#define MAYBE_SIMILAR_LIST_SET(t,x,y,r) \
((t) == TypeListSet ? \
 ((ListSet*)(x))->similar((ListSet*)(y)) : r)
#define MAYBE_LIST_SET_UNION(x,y)\
    case TypeListSet:\
      ((ListSet*)(x))->unioni((ListSet*)(y));\
      break;
#else
#define MAYBE_CREATE_LIST_SET(x,r) (r)
#define MAYBE_SIMILAR_LIST_SET(t,x,y,r) (r)
#define MAYBE_LIST_SET_UNION(x,y)
#endif

#ifdef USE_RANGE_SETS
#include "RangeSet.h"
#define MAYBE_CREATE_RANGE_SET(x,r) ((x) == TypeRangeSet ? NEW(RangeSet) : (r))
#define MAYBE_SIMILAR_RANGE_SET(t,x,y,r) \
((t) == TypeRangeSet ? \
 ((RangeSet*)(x))->similar((RangeSet*)(y)) : r)
#define MAYBE_RANGE_SET_UNION(x,y)\
    case TypeRangeSet:\
      ((RangeSet*)(x))->unioni((RangeSet*)(y));\
      break;
#else
#define MAYBE_CREATE_RANGE_SET(x,r) (r)
#define MAYBE_SIMILAR_RANGE_SET(t,x,y,r) (r)
#define MAYBE_RANGE_SET_UNION(x,y)
#endif

#ifdef UNIQUE_SUCCESSOR_SET_TYPE

#ifdef USE_BIT_SETS
class BitSet;
class BitSetIter;
typedef BitSet SuccessorSet;
typedef BitSetIter SuccessorSetIter;
#endif
#ifdef USE_CHAIN_SETS
class ChainSet;
class ChainSetIter;
typedef ChainSet SuccessorSet;
typedef ChainSetIter SuccessorSetIter;
#endif
#ifdef USE_INTERVAL_SETS
class IntervalSet;
class IntervalSetIter;
typedef IntervalSet SuccessorSet;
typedef IntervalSetIter SuccessorSetIter;
#endif
#ifdef USE_LIST_BIT_SETS
class ListBitSet;
class ListBitSetIter;
typedef ListBitSet SuccessorSet;
typedef ListBitSetIter SuccessorSetIter;
#endif
#ifdef USE_LIST_SETS
class ListSet;
class ListSetIter;
typedef ListSet SuccessorSet;
typedef ListSetIter SuccessorSetIter;
#endif
#ifdef USE_RANGE_SETS
class RangeSet;
class RangeSetIter;
typedef RangeSet SuccessorSet;
typedef RangeSetIter SuccessorSetIter;
#endif

#define create_successor_set(x) NEW(SuccessorSet)
#define successor_set_union(x,y,type) (x)->unioni(y)
#define successor_set_similar(x,x_type,y,y_type) (x)->similar(y)
#define successor_set_similar1(x,y,type) (x)->similar(y)

#else

typedef NatSet SuccessorSet;
typedef NatSetIter SuccessorSetIter;

#define create_successor_set(x)\
MAYBE_CREATE_INTERVAL_SET(x,\
MAYBE_CREATE_RANGE_SET(x,\
MAYBE_CREATE_CHAIN_SET(x,\
MAYBE_CREATE_LIST_BIT_SET(x,\
MAYBE_CREATE_LIST_SET(x,\
MAYBE_CREATE_BIT_SET(x,\
((SuccessorSet*)(printf_abort("Illegal successor set type\n", 0)))))))))

#define successor_set_union(x,y,type)\
do {\
    switch (type) {\
    MAYBE_INTERVAL_SET_UNION(x,y)\
    MAYBE_RANGE_SET_UNION(x,y)\
    MAYBE_CHAIN_SET_UNION(x,y)\
    MAYBE_LIST_BIT_SET_UNION(x,y)\
    MAYBE_LIST_SET_UNION(x,y)\
    MAYBE_BIT_SET_UNION(x,y)\
    default:\
      printf_abort("Illegal successor set type\n");\
    }\
} while(0)

#define successor_set_similar(x,x_type,y,y_type)\
  ((x) == 0 ? (y) == 0 : \
   ((y) == 0 ? 0 : \
    (x_type != y_type ? (x)->similar((y)) :\
     MAYBE_SIMILAR_INTERVAL_SET(x_type,x,y,\
      MAYBE_SIMILAR_RANGE_SET(x_type,x,y,\
       MAYBE_SIMILAR_CHAIN_SET(x_type,x,y,\
	MAYBE_SIMILAR_LIST_BIT_SET(x_type,x,y,\
	 MAYBE_SIMILAR_LIST_SET(x_type,x,y,\
	  MAYBE_SIMILAR_BIT_SET(x_type,x,y,\
	   (printf_abort("Illegal successor set type\n", 0)))))))))))

#define successor_set_similar1(x,y,type)\
  MAYBE_SIMILAR_INTERVAL_SET(type,x,y,\
   MAYBE_SIMILAR_RANGE_SET(type,x,y,\
    MAYBE_SIMILAR_CHAIN_SET(type,x,y,\
     MAYBE_SIMILAR_LIST_BIT_SET(type,x,y,\
      MAYBE_SIMILAR_LIST_SET(type,x,y,\
       MAYBE_SIMILAR_BIT_SET(type,x,y,\
        (printf_abort("Illegal successor set type\n", 0))))))))

#endif

#endif

#endif
