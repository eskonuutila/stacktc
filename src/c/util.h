/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: util.h

  Auxiliary functions.
  =============================================================================
*/

/* -*- Mode: C -*- */

#ifndef _util_h_
#define _util_h_

#include "types.h"
#include "macros.h"

vint *new_vint_table(vint nelem, vint init);
int cmp_vint(const void *a, const void *b);

#endif
