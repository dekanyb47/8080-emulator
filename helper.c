#include <stdio.h>
#include <stdlib.h>

#include "helper.h"

void die(const char *s){
  printf("Error for the following function based on the errno value set:\n");
  perror(s);
  exit(1);
}