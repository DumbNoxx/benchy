#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "childs/sum.h"

int main(int argc, char *argv[])
{
  double time_duration, time_wall;
  struct timespec ts1, tw1, ts2, tw2;
  clock_gettime(CLOCK_MONOTONIC, &tw1);
  printf("argumentos nuermo: %d\n", argc);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts1);

  for (int i = 1; i < argc; i++)
  {
    pid_t pid = fork();

    if (pid == 0)
    {
        childSum(i * i+1, i * i + 2, i, argv[i]);
        return 0;
    }
    else if (pid < 0)
    {
      perror("error fork");
      return 1;
    }
  }
  while (wait(NULL) != -1 || errno != ECHILD)
  {
    printf("Waited for a child to finish\n");
  }
  clock_gettime(CLOCK_MONOTONIC, &tw2);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts2);

  time_duration = (ts2.tv_sec - ts1.tv_sec) * 1000.0 +
                  (double)(ts2.tv_nsec - ts1.tv_nsec) / 1e6;

  printf("CPU time used (per clock_gettime()): %.2f ms\n", time_duration);
  time_wall = (tw2.tv_sec - tw1.tv_sec) * 1000.0 +
              (double)(tw2.tv_nsec - tw1.tv_nsec) / 1e6;

  printf("Wall time passed: %.2f ms\n", time_wall);
  return 0;
}
