#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
  int fd  = open(argv[1], O_RDONLY);
  int n;
  char buf[1024];

  n = read(fd, buf, sizeof(buf)-1);
  buf[n] = '\0';

  printf("++++ testopen.c: open returned %d: %s\n", fd, buf);
}

