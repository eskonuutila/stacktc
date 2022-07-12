/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: warshall.h

  Computing the transitive closure of a matrix using Warshall's algorithm. This
  is much slower than the algorithm stacktc, but needed for checking the result
  =============================================================================
*/

#ifndef _warshall_h_
#define _warshall_h_

#include "types.h"
#include "macros.h"

Matrix *warshall(Matrix *this);
#endif
