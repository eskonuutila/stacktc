/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: util.c

  Auxiliary functions.
  =============================================================================
*/

#include "util.h"

/* Allocate and initialize an array of vints */
vint *new_vint_table(vint nelem, vint init) {
  vint *table = NEWN(vint, nelem);
  vint i;
  for (i = 0; i < nelem; i++) {
    table[i] = init;
  }
  return table;
}

/* Compare two vints. Used as a parameter for qsort */
int cmp_vint(const void *a, const void *b) {
  return (*((vint*)a) - *((vint*)b));
}
