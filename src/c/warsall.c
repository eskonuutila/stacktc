/*
  =============================================================================
  Author:  Esko Nuutila (enu@iki.fi)
  Date:    2017-06-23
  Date:    2022-07-12
  Licence: MIT
  =============================================================================
  File: warshall.c

  Computing the transitive closure of a matrix using Warshall's algorithm. This
  is much slower than the algorithm stacktc, but needed for checking the result
  =============================================================================
*/

#include "warshall.h"

Matrix *warshall(Matrix *this) {
  vint i, j, k;
  vint n = this->n;
  Matrix *result = NEW(Matrix);
  vint *elements = NEWN(vint, n*n);
  memcpy(elements, this->elements, sizeof(vint)*n*n);
  for (k = 0; k < n; k++) {
    for (i = 0; i < n; i++) {
      if (i != k && elements[i*n + k]) {
	for (j = 0; j < n; j++) {
	  if (elements[k*n + j]) {
	    elements[i*n + j] = 1;
	  }
	}
      }
    }
  }
  result->n = n;
  result->elements = elements;
  return result;
}

