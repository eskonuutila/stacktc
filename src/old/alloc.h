#ifndef _alloc_h_

#define NEW(PTRTYPE,NELEMS) ((PTRTYPE)malloc(sizeof(PTRTYPE*)*NELEMS))
#define DELETE(X) (free(X))

#endif
