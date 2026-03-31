#include "childs/sum.h"
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "utils/utils.h"


void childSum(int a, int b, int i, char* arg)
{
  struct timespec h_ts1, h_ts2;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &h_ts1);
  printf("Child: %d: working (PID: %d, Argument: %s)\n", i, getpid(), arg);

  int result = suma(a * i, b * i);
  sleep(1);

  printf("Result: %d\n", result);


  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &h_ts2);
  double h_duration = (h_ts2.tv_sec - h_ts1.tv_sec) * 1000.0 +
                      (h_ts2.tv_nsec - h_ts1.tv_nsec) / 1e6;
  printf("Child: %d, CPU time used: %.2f ms\n\n", i, h_duration);
}
