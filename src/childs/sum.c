#define _GNU_SOURCE
#include "childs/sum.h"
#include "utils/utils.h"
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void childSum(int a, int b, int i, char *arg, int fd)
{
  if (unshare(CLONE_NEWNS | CLONE_NEWUSER | CLONE_NEWNET) == -1)
  {
    perror("unshare");
    exit(1);
  }
  struct timespec h_ts1, h_ts2;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &h_ts1);
  int pid = getpid();
  char *argument = arg;

  if (write(fd, &pid, sizeof(int)) == -1)
  {
    perror("error pipe");
    exit(1);
  }
  if (write(fd, &arg, sizeof(argument)) == -1)
  {
    perror("error pipe");
    exit(1);
  }

  int result = suma(a * i, b * i);

  if (write(fd, &result, sizeof(int)) == -1)
  {
    perror("error pipe");
  }

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &h_ts2);
  double h_duration = (h_ts2.tv_sec - h_ts1.tv_sec) * 1000.0 +
                      (h_ts2.tv_nsec - h_ts1.tv_nsec) / 1e6;

  if (write(fd, &h_duration, sizeof(double)) == -1)
  {
    perror("error pipe");
    exit(1);
  }
}
