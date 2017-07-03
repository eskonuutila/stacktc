#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  FILE *input = fopen(argv[1], "r");
  char buffer[101];
  int from, to;
  fprintf(stderr, "input: %d\n", ftell(input));
  if (fscanf(input, "%s\n", buffer) != 1) {
    fprintf(stderr, "Could not read line\n");
    exit(1);
  } else {
    char *comma = strchr(buffer, ',');
    char *field1, *field2;
    fprintf(stderr, "Line1 = '%s'\n", buffer);
    if (comma != NULL) {
      field1 = buffer;
      *comma = (char)0;
      field2 = comma + 1;
      fprintf(stderr, "'%s', '%s'\n", field1, field2);
    }
  }
  
  while (fscanf(input, "%d,%d\n", &from, &to) == 2) {
    fprintf(stderr, "Line = '%d','%d'\n", from, to);
  }
  return 0;
}
