/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: intervals.c

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

#include "intervals.h"

Interval *interval_table_from = 0;
Interval *interval_table_to = 0;

void Intervals_initialize_tc(vint max_ids) {
  interval_table_to = NEWN(Interval, max_ids/2+1);
  interval_table_from = NEWN(Interval, max_ids/2+1);
}

/* Create a new interval set. */
Intervals *Intervals_new() {
  Intervals *this = NEW(Intervals);
  this->interval_count = 0;
  this->interval_table = interval_table_from;
  return this;
}

/* This function is needed because of the storage method used */
void Intervals_completed(Intervals *this) {
  Assert(this->interval_table == interval_table_from);
  Interval *ins = NEWN(Interval, this->interval_count);
  memcpy(ins, this->interval_table, sizeof(Interval)*this->interval_count);
  this->interval_table = ins;
}

/* Inserting a number to an interval set. This may extend an existing interval,
   generate a new interval, or do nothing if the number already is in the interval set */
vint Intervals_insert(Intervals *this, vint id) {
  vint min = 0;
  vint max = this->interval_count - 1;
  Interval *ins = this->interval_table;
  if (min > max) {
    ins[0].low = ins[0].high = id;
    this->interval_count = 1;
    return 0;
  } else {
    do {
      vint index = (max + min)/2;
      Interval *elem = &(ins[index]);
      if (id < elem->low)
	max = index - 1;
      else if (id > elem->high)
	min = index + 1;
      else {
	return 1;
      }
    } while (min <= max);
  }
  if (max < 0) {
    if (id == ins[0].low - 1) {
      ins[0].low--;
    } else {
      vint i;
      for (i = this->interval_count - 1; i >= 0; i--)
	ins[i+1] = ins[i];
      ins[0].low = ins[0].high = id;
      this->interval_count++;
    }
  } else if (min == this->interval_count) {
    if (id == ins[min - 1].high + 1)
      ins[min - 1].high++;
    else {
      ins[min].low = ins[min].high = id;
      this->interval_count++;
    }
  } else {
    if (id == ins[max].high + 1) {
      if (id == ins[min].low - 1) {
	ins[max].high = ins[min].high;
	vint i;
	for (i = min; i < this->interval_count - 1; i++)
	  ins[i] = ins[i + 1];
	this->interval_count--;
      } else {
	ins[max].high++;
      }
    } else if (id == ins[min].low - 1) {
      ins[min].low--;
    } else {
      vint i;
      for (i = this->interval_count - 1; i >= min; i--)
	ins[i+1] = ins[i];
      ins[min].low = ins[min].high = id;
      this->interval_count++;
    }
  }
  return 0;
}

/* The union of two interval sets. Note that the result may contain
   a smaller number of intervals than either of the parameters */
void Intervals_union(Intervals *this, Intervals *other) {
  if (!other || other->interval_count == 0) return;
  Interval *result = interval_table_to;
  Interval *ins1 = this->interval_table;
  Interval *ins2 = other->interval_table;
  vint i1 = 0, i2 = 0, i = 0;
  vint max1 = this->interval_count;
  vint max2 = other->interval_count;
  if (max1 == 0)
    goto copy2;
  while (1) {
    if (ins1[i1].high < ins2[i2].low - 1) {
      result[i++] = ins1[i1++];
      if (i1 == max1)
	goto copy2;
    } else if (ins2[i2].high < ins1[i1].low - 1) {
      result[i++] = ins2[i2++];
      if (i2 == max2)
	goto copy1;
    } else {
      if (ins1[i1].low < ins2[i2].low)
	result[i].low = ins1[i1].low;
      else
	result[i].low = ins2[i2].low;
      vint h;
      if (ins1[i1].high > ins2[i2].high) {
	h = ins1[i1].high;
	i1++; i2++;
	goto ahead1;
      } else if (ins1[i1].high < ins2[i2].high) {
	h = ins2[i2].high;
	i1++; i2++;
	goto ahead2;
      } else {
	result[i].high = ins1[i1].high;
	i1++; i2++; i++;
	if (i1 == max1)
	  goto copy2;
	if (i2 == max2)
	  goto copy1;
	continue;
      }
    ahead1:
      while (i2 < max2 && ins2[i2].high <= h)
	i2++;
      if (i2 == max2) {
	result[i++].high = h;
	goto copy1;
      } else if (ins2[i2].low - 1 <= h) {
	h = ins2[i2++].high;
	goto ahead2;
      } else {
	result[i++].high = h;
	if (i1 == max1)
	  goto copy2;
	continue;
      }
    ahead2:
      while (i1 < max1 && ins1[i1].high <= h)
	i1++;
      if (i1 == max1) {
	result[i++].high = h;
	goto copy2;
      } else if (ins1[i1].low - 1 <= h) {
	h = ins1[i1++].high;
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
    result[i++] = ins1[i1++];
  goto done;
 copy2:
  while (i2 < max2)
    result[i++] = ins2[i2++];
 done:
  interval_table_to = ins1;
  this->interval_table = interval_table_from = result;
  this->interval_count = i;
}

/* Find a number in an interval set */
vint Intervals_find(Intervals *this, vint id) {
  vint min = 0;
  vint max = this->interval_count - 1;
  while (min <= max) {
    vint index = (max + min)/2;
    Interval *elem = &(this->interval_table[index]);
    if (id < elem->low)
      max = index - 1;
    else if (id > elem->high)
      min = index + 1;
    else {
      return 1;      
    }
  }
  return 0;
}
