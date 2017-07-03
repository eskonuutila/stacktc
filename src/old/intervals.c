// -*- Mode: C++ -*-

#include "IntervalSet.h"

interval_set *interval_set_new() {
  this = NEW(interval_set*);
  this->interval_count = 0;
  Assert(interval_set_s_from);
  this->intervals = interval_set_s_from;
}

void interval_set *interval_set_delete(interval_set* this) {
  this->intervals = 0;
  DELETE(this);
}

void interval_set_clear(interval_set* this) {
    this->intervals = 0;
    this->interval_count = 0;
}

int interval_set_empty(interval_set* this) {
    return this->interval_count == 0;
}

int interval_set_size(interval_set* this) {
  // This could be a variable; thus, only a constant cost
  int sum = this->interval_count;
  int i;
  for (i = 0; i < this->interval_count; i++)
    sum += this->intervals[i].high - this->intervals[i].low;
  return sum;
}

int interval_set_find(interval_set* this, int id) {
  int min = 0;
  int max = this->interval_count - 1;
  while (min <= max) {
    int index = (max + min)/2;
    interval *elem = &(this->intervals[index]);
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

int interval_set_insert(interval_set* this, int id) {
  int min = 0;
  int max = this->interval_count - 1;
  if (min > max) {
    this->intervals[0].low = this->intervals[0].high = id;
    this->interval_count = 1;
    return 0;
  } else {
    do {
      int index = (max + min)/2;
      interval *elem = &(this->intervals[index]);
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
    if (id == this->intervals[0].low - 1) {
      this->intervals[0].low--;
    } else {
      int i;
      for (i = this->interval_count - 1; i >= 0; i--)
	this->intervals[i+1] = this->intervals[i];
      this->intervals[0].low = this->intervals[0].high = id;
      this->interval_count++;
    }
  } else if (min == this->interval_count) {
    if (id == this->intervals[min - 1].high + 1)
      this->intervals[min - 1].high++;
    else {
      this->intervals[min].low = this->intervals[min].high = id;
      this->interval_count++;
    }
  } else {
    if (id == this->intervals[max].high + 1) {
      if (id == this->intervals[min].low - 1) {
	this->intervals[max].high = this->intervals[min].high;
	int i;
	for (i = min; i < this->interval_count - 1; i++)
	  this->intervals[i] = this->intervals[i + 1];
	this->interval_count--;
      } else {
	this->intervals[max].high++;
      }
    } else if (id == this->intervals[min].low - 1) {
      this->intervals[min].low--;
    } else {
      int i;
      for (i = this->interval_count - 1; i >= min; i--)
	this->intervals[i+1] = this->intervals[i];
      this->intervals[min].low = this->intervals[min].high = id;
      this->interval_count++;
    }
  }
  return 0;
}

void interval_set_union(interval_set* this, interval_set* other) {
  if (!other || other->interval_count == 0) return;
  interval *result = s_to;
  interval *in1 = this->intervals;
  interval *in2 = other->intervals;
  int i1 = 0, i2 = 0, i = 0;
  int max1 = this->interval_count;
  int max2 = other->interval_count;
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
  this->intervals = s_from = result;
  this->interval_count = i;
}

void interval_set_complete(interval_set* this) {
  Assert(this->intervals == s_from);
  interval *intervals = (interval*)s_allocator->allocate(this->interval_count);
  int i;
  for (i = 0; i < this->interval_count; i++)
    intervals[i] = this->intervals[i];
  this->intervals = intervals;
}

int interval_set_similar(interval_set* this, interval_set* other) {
  if (!this) 
    return (!other || other->interval_count == 0);
  else if (!other || this->interval_count != other->interval_count)
    return 0;
  else {
    int i;
    for (i = 0; i < this->interval_count; i++)
      if (this->intervals[i].low != other->intervals[i].low ||
	  this->intervals[i].high != other->intervals[i].high)
	return 0;
    return 1;
  }
}

void interval_set_print(interval_set* this, FILE *stream) {
  if (!this) {
    fprintf(stream, "NULL");
    return;
  }
  fprintf(stream, "{");
  int firstp = 1;
  int i;
  for (i = 0; i < this->interval_count; i++)
    int v;
    for (v = this->intervals[i].low; v <= this->intervals[i].high; v++)
      if (firstp) {
	firstp = 0;
	fprintf(stream, "%d", v);
      } else
	fprintf(stream, ", %d", v);
  fprintf(stream, "}");
}

void interval_set_print_structure(interval_set* this, FILE *stream, int) {
  if (!this) {
    fprintf(stream, "NULL");
    return;
  }
  fprintf(stream, "{s=%d,c=%d: ", size(), this->interval_count);
  int firstp = 1;
  int i;
  for (i = 0; i < this->interval_count; i++)
    if (firstp) {
      firstp = 0;
      fprintf(stream, "[%d, %d]", 
	      this->intervals[i].low, this->intervals[i].high);
    } else
      fprintf(stream, ",[%d, %d]", 
	      this->intervals[i].low, this->intervals[i].high);
  fprintf(stream, "}");
}

void interval_set_check(interval_set* this) {
  if (this->interval_count > 0) {
    if (this->intervals[0].low > this->intervals[0].high) {
      fprintf(stderr, "Interval corrupted at index 0\n");
      abort();
      return 0;
    }
    int i;
    for (i = 0; i < this->interval_count - 1; i++) {
      if (this->intervals[i+1].low > this->intervals[i+1].high) {
	fprintf(stderr, "Interval corrupted at index %d\n", i);
	abort();
	return 0;
      }
      if (this->intervals[i].high >= this->intervals[i+1].low - 1) {
	fprintf(stderr, "Intervals corrupted at index %d (+1)\n", i);
	abort();
	return 0;
      }
    }
  }
  return 1;
}
