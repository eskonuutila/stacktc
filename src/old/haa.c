#include <stdio.h>

int main() {
  typedef struct DM {
    int i;
    double d;
  } THE, *THEPTR;

  printf("sizeof(THE)=%ld, sizeof(THEPTR)=%ld, sizeof(THEPTR*)=%ld\n",
	 sizeof(THE), sizeof(THEPTR), sizeof(THEPTR*));
  printf("apina" "on\n");

}
