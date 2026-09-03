#include <stdio.h>
#include <stdlib.h>

#include "helper.h"

void die(const char *s){
  perror(s);
  exit(1);
}