#include <stdio.h>
#include <stdlib.h>

#include "helper.h"

int read_file_to_buf(const char *filepath, char ** const buf, long *bufsize) {
  FILE *fp = fopen(filepath, "rb");
  if (fp == NULL) die("fopen");

  // get the length of the file
  if (fseek(fp, 0L, SEEK_END) != 0) die("fseek");
  const long bufsize_tmp = ftell(fp);
  if (bufsize_tmp < 0) die("ftell");

  char *buf_tmp = malloc(sizeof(char) * bufsize_tmp);
  if (buf_tmp == NULL) die("malloc");

  // set fp back to the beginning of the file
  if (fseek(fp, 0L, SEEK_SET) != 0) die("fseek");

  // TODO: commented line doesn't work!
  // if (fgets(buf_tmp, sizeof(char) * (bufsize_tmp + 1), fp) == NULL) return 1;
  for (long i = 0; i < bufsize_tmp; i++) {
    char c = fgetc(fp);
    buf_tmp[i] = c;
  }
  
  *bufsize = bufsize_tmp;
  *buf = buf_tmp;

  return 0;
}

void die(const char *s){
  perror(s);
  exit(1);
}