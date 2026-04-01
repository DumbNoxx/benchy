#include "childs/sum.h"
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  double time_duration, time_wall;
  struct timespec ts1, tw1, ts2, tw2;
  clock_gettime(CLOCK_MONOTONIC, &tw1);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts1);

  int num_arg = argc - 1, pipes[num_arg][2];

  for (int i = 0; i < num_arg; i++)
  {
    if (pipe(pipes[i]) == -1)
    {
      printf("An error ocurred with opening the pipe\n");
      perror("pipe");
      return 1;
    }
    pid_t pid = fork();

    if (pid == 0)
    {
      close(pipes[i][0]);
      childSum(i * i + 1, i * i + 2, i, argv[i + 1], pipes[i][1]);
      close(pipes[i][1]);
      return 0;
    }
    else if (pid != 0)
    {
      close(pipes[i][1]);
      int result, pid;
      double h_duration;
      char *argument;
      if (read(pipes[i][0], &pid, sizeof(int)) == -1)
      {
        perror("Error to read pipe");
        return 1;
      }
      if (read(pipes[i][0], &argument, sizeof(argument)) == -1)
      {
        perror("Error to read pipe");
        return 1;
      }

      printf("Child: %d: working (PID: %d, Argument: %s)\n", i, pid, argument);
      if (read(pipes[i][0], &result, sizeof(int)) == -1)
      {
        perror("Error to read pipe");
        return 1;
      }
      printf("Result sum: %d\n", result);
      if (read(pipes[i][0], &h_duration, sizeof(double)) == -1)
      {
        perror("Error to read pipe");
        return 1;
      }
      printf("Child: %d, CPU time used: %.2f ms\n\n", i, h_duration);
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

  time_duration =
      (ts2.tv_sec - ts1.tv_sec) * 1000.0 + (ts2.tv_nsec - ts1.tv_nsec) / 1e6;

  printf("CPU time used (per clock_gettime()): %.2f ms\n", time_duration);
  time_wall =
      (tw2.tv_sec - tw1.tv_sec) * 1000.0 + (tw2.tv_nsec - tw1.tv_nsec) / 1e6;

  printf("Wall time passed: %.2f ms\n", time_wall);
  return 0;
}
