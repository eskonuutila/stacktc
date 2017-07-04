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
// This file implements classes IntervalSet and IntervalSetIter
//
// Ifdefs:
//  
//   COLLECT_SET_METRICS: if non-zero, inserts code for collecting set metrics.
//   NOTE! You must call set_costing(1) somewhere!
//
//   DEBUG_INTERVALSET: if non-zero, debug information is printed.
//

#include "IntervalSet.h"

#ifdef DEBUG_INTERVALSET
#define BARF(x) fprintf x;
#define BARF_STMT(x) x
#else
#define BARF(x)
#define BARF_STMT(x)
#endif

SET_METRICS_DEFINE(interval);

DEFINE_ALLOC_ROUTINES(IntervalSet, 1024, 128);

DEFINE_ALLOC_ROUTINES(IntervalSetIter, 1024, 128);

Interval *IntervalSet::s_from = 0;
Interval *IntervalSet::s_to = 0;
BlockAllocator *IntervalSet::s_allocator = 0;

void IntervalSet::initialize_closure(int max_ids) {
  s_allocator = NEW(BlockAllocator(sizeof(Interval), max_ids));
  s_to = (Interval*)s_allocator->allocate(max_ids/2+1);
  s_from = (Interval*)s_allocator->allocate(max_ids/2+1);
}

int IntervalSet::size() {
  // This could be a variable; thus, only a constant cost
  INSERT_SET_SAMPLE(interval_set_size_metric, interval_set_size_cost);
  int sum = v_interval_count;
  for (int i = 0; i < v_interval_count; i++)
    sum += v_intervals[i].high - v_intervals[i].low;
  return sum;
}

int IntervalSet::find(int id) {
  int min = 0;
  int max = v_interval_count - 1;
#ifdef COLLECT_SET_METRICS
  int cycles = 1;
#endif
  while (min <= max) {
    int index = (max + min)/2;
    Interval *elem = &(v_intervals[index]);
    if (id < elem->low)
      max = index - 1;
    else if (id > elem->high)
      min = index + 1;
    else {
      INSERT_SET_SAMPLE(interval_set_find_metric, cycles*interval_set_find_cost);
      return 1;      
    }
#ifdef COLLECT_SET_METRICS
    cycles++;
#endif
  }
  INSERT_SET_SAMPLE(interval_set_find_metric, cycles*interval_set_find_cost);
  return 0;
}

int IntervalSet::insert(int id) {
  int min = 0;
  int max = v_interval_count - 1;
#ifdef COLLECT_SET_METRICS
  int cycles = 1;
#endif
  if (min > max) {
    v_intervals[0].low = v_intervals[0].high = id;
    v_interval_count = 1;
    INSERT_SET_SAMPLE(interval_set_insert_metric, interval_set_insert_cost);
    INSERT_SET_SAMPLE(interval_set_insert_found_metric, 0);
    return 0;
  } else {
    do {
      int index = (max + min)/2;
      Interval *elem = &(v_intervals[index]);
      if (id < elem->low)
	max = index - 1;
      else if (id > elem->high)
	min = index + 1;
      else {
	INSERT_SET_SAMPLE(interval_set_insert_found_metric, 1);
	INSERT_SET_SAMPLE(interval_set_insert_metric, cycles*interval_set_insert_cost);
	return 1;
      }
#ifdef COLLECT_SET_METRICS
      cycles++;
#endif
    } while (min <= max);
  }
  if (max < 0) {
    if (id == v_intervals[0].low - 1) {
      v_intervals[0].low--;
#ifdef COLLECT_SET_METRICS
      cycles++;
#endif
    } else {
      for (int i = v_interval_count - 1; i >= 0; i--)
	v_intervals[i+1] = v_intervals[i];
      v_intervals[0].low = v_intervals[0].high = id;
      v_interval_count++;
#ifdef COLLECT_SET_METRICS
      cycles += v_interval_count;
#endif
    }
  } else if (min == v_interval_count) {
    if (id == v_intervals[min - 1].high + 1)
      v_intervals[min - 1].high++;
    else {
      v_intervals[min].low = v_intervals[min].high = id;
      v_interval_count++;
    }
#ifdef COLLECT_SET_METRICS
    cycles++;
#endif
  } else {
    if (id == v_intervals[max].high + 1) {
      if (id == v_intervals[min].low - 1) {
	v_intervals[max].high = v_intervals[min].high;
	for (int i = min; i < v_interval_count - 1; i++)
	  v_intervals[i] = v_intervals[i + 1];
	v_interval_count--;
#ifdef COLLECT_SET_METRICS
	cycles += v_interval_count - min;
#endif
      } else {
	v_intervals[max].high++;
#ifdef COLLECT_SET_METRICS
	cycles++;
#endif
      }
    } else if (id == v_intervals[min].low - 1) {
      v_intervals[min].low--;
#ifdef COLLECT_SET_METRICS
      cycles++;
#endif
    } else {
      for (int i = v_interval_count - 1; i >= min; i--)
	v_intervals[i+1] = v_intervals[i];
      v_intervals[min].low = v_intervals[min].high = id;
      v_interval_count++;
#ifdef COLLECT_SET_METRICS
      cycles += v_interval_count - min;
#endif
    }
  }
  INSERT_SET_SAMPLE(interval_set_insert_metric, cycles*interval_set_insert_cost);
  INSERT_SET_SAMPLE(interval_set_insert_found_metric, 0);
  return 0;
}

#ifdef SUPER_OPS
extern int superp;
#endif

void IntervalSet::unioni(IntervalSet* other) {
#ifdef SUPER_OPS
  if (superp) {
    fprintf(stderr, "Supercomponent union operation:\n  Super->succ() == ");
    print_structure(stderr);
    fprintf(stderr, "\n  other = ");
    if (other)
      other->print_structure(stderr);
    else
      fprintf(stderr, "NULL");
    fprintf(stderr, "\n");
  }
#endif
  if (!other || other->v_interval_count == 0) return;
  Interval *result = s_to;
  Interval *in1 = v_intervals;
  Interval *in2 = other->v_intervals;
  int i1 = 0, i2 = 0, i = 0;
  int max1 = v_interval_count;
  int max2 = other->v_interval_count;
  INSERT_SET_SAMPLE(interval_set_unioni_metric, (max1+max2)*interval_set_unioni_cost);
#ifdef SUPER_OPS
  if (superp)
    fprintf(stderr, "cost = %d + %d = %d\n", max1, max2, max1 + max2);
#endif
  if (max1 == 0)
    goto copy2;
  while (1) {
    if (in1[i1].high < in2[i2].low - 1) {
      result[i++] = in1[i1++];
      if (i1 == max1)
	goto copy2;
    } else if (in2[i2].high < in1[i1].low - 1) {
      result[i++] = in2[i2++];
      if (i2 == max2)
	goto copy1;
    } else {
      if (in1[i1].low < in2[i2].low)
	result[i].low = in1[i1].low;
      else
	result[i].low = in2[i2].low;
      int h;
      if (in1[i1].high > in2[i2].high) {
	h = in1[i1].high;
	i1++; i2++;
	goto ahead1;
      } else if (in1[i1].high < in2[i2].high) {
	h = in2[i2].high;
	i1++; i2++;
	goto ahead2;
      } else {
	result[i].high = in1[i1].high;
	i1++; i2++; i++;
	if (i1 == max1)
	  goto copy2;
	if (i2 == max2)
	  goto copy1;
	continue;
      }
    ahead1:
      while (i2 < max2 && in2[i2].high <= h)
	i2++;
      if (i2 == max2) {
	result[i++].high = h;
	goto copy1;
      } else if (in2[i2].low - 1 <= h) {
	h = in2[i2++].high;
	goto ahead2;
      } else {
	result[i++].high = h;
	if (i1 == max1)
	  goto copy2;
	continue;
      }
    ahead2:
      while (i1 < max1 && in1[i1].high <= h)
	i1++;
      if (i1 == max1) {
	result[i++].high = h;
	goto copy2;
      } else if (in1[i1].low - 1 <= h) {
	h = in1[i1++].high;
	goto ahead1;
      } else {
	result[i++].high = h;
	if (i2 == max2)
	  goto copy1;
	continue;
      }
    }
  }
 copy1:
  while (i1 < max1)
    result[i++] = in1[i1++];
  goto done;
 copy2:
  while (i2 < max2)
    result[i++] = in2[i2++];
 done:
  s_to = in1;
  v_intervals = s_from = result;
  v_interval_count = i;
}

void IntervalSet::complete() {
  Assert(v_intervals == s_from);
  Interval *intervals = (Interval*)s_allocator->allocate(v_interval_count);
  for (int i = 0; i < v_interval_count; i++)
    intervals[i] = v_intervals[i];
  v_intervals = intervals;
}

int IntervalSet::similar(IntervalSet* other) {
  if (!this) 
    return (!other || other->v_interval_count == 0);
  else if (!other || v_interval_count != other->v_interval_count)
    return 0;
  else {
    for (int i = 0; i < v_interval_count; i++)
      if (v_intervals[i].low != other->v_intervals[i].low ||
	  v_intervals[i].high != other->v_intervals[i].high)
	return 0;
    return 1;
  }
}

void IntervalSet::print(FILE *stream) {
  if (!this) {
    fprintf(stream, "NULL");
    return;
  }
  fprintf(stream, "{");
  int firstp = 1;
  for (int i = 0; i < v_interval_count; i++)
    for (int v = v_intervals[i].low; v <= v_intervals[i].high; v++)
      if (firstp) {
	firstp = 0;
	fprintf(stream, "%d", v);
      } else
	fprintf(stream, ", %d", v);
  fprintf(stream, "}");
}

void IntervalSet::print_structure(FILE *stream, int) {
  if (!this) {
    fprintf(stream, "NULL");
    return;
  }
  fprintf(stream, "{s=%d,c=%d: ", size(), v_interval_count);
  int firstp = 1;
  for (int i = 0; i < v_interval_count; i++)
    if (firstp) {
      firstp = 0;
      fprintf(stream, "[%d, %d]", 
	      v_intervals[i].low, v_intervals[i].high);
    } else
      fprintf(stream, ",[%d, %d]", 
	      v_intervals[i].low, v_intervals[i].high);
  fprintf(stream, "}");
}

IntervalSetIter::IntervalSetIter(IntervalSet *s, int clearp, int reversep) {
  INSERT_SET_SAMPLE(interval_set_iter_init_metric, interval_set_iter_init_cost);
  v_set = s;
  v_clearp = clearp;
  v_reversep = reversep;
  if (reversep) {
    v_index_limit = -1;
    v_index = s->v_interval_count;
    v_value = v_value_limit = 0;
  } else {
    v_index_limit = s->v_interval_count;
    v_index = -1;
    v_value = v_value_limit = 0;
  }
}

int IntervalSetIter::next() {
  INSERT_SET_SAMPLE(interval_set_iter_next_metric, interval_set_iter_next_cost);
  if (v_reversep) {
    if (v_value == v_value_limit) {
      if (--v_index <= v_index_limit)
	return -1;
      else {
	v_value = v_set->v_intervals[v_index].high + 1;
	v_value_limit = v_set->v_intervals[v_index].low;
      }
    }
    return --v_value;
  } else {
    if (v_value == v_value_limit) {
      if (++v_index >= v_index_limit)
	return -1;
      else {
	v_value = v_set->v_intervals[v_index].low - 1;
	v_value_limit = v_set->v_intervals[v_index].high;
      }
    }
    return ++v_value;
  }
}

int IntervalSet::check() {
  if (v_interval_count > 0) {
    if (v_intervals[0].low > v_intervals[0].high) {
      fprintf(stderr, "Interval corrupted at index 0\n");
      abort();
      return 0;
    }
    for (int i = 0; i < v_interval_count - 1; i++) {
      if (v_intervals[i+1].low > v_intervals[i+1].high) {
	fprintf(stderr, "Interval corrupted at index %d\n", i);
	abort();
	return 0;
      }
      if (v_intervals[i].high >= v_intervals[i+1].low - 1) {
	fprintf(stderr, "Intervals corrupted at index %d (+1)\n", i);
	abort();
	return 0;
      }
    }
  }
  return 1;
}
