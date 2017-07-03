/* -*- Mode: C -*- */

#ifndef _interval_set_h
#define _interval_set_h

#include <stdio.h>
#include "config.h"
#include "util.h"
#include "allocs.h"
#include "NatSet.h"
#include "Metric.h"
#include "set-metrics.h"

typedef struct interval_struct {
  int low, high;
} *interval;

typedef struct interval_set_struct {
  interval *intervals;
  int interval_count;
} *interval_set;

interval_set_s_from = 0;
interval_set_s_to = 0;

#endif
