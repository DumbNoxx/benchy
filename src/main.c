#include "childs/sum.h"
#include "utils/utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("At least one argument is required\n");
    return 1;
  }
  const char *cgroup_dir = "/sys/fs/cgroup/benchy";
  int ramUsage30Percent = prepareTmp(cgroup_dir);
  int pid_rm[argc - 1];
  double time_duration, time_wall;
  struct timespec ts1, tw1, ts2, tw2;
  clock_gettime(CLOCK_MONOTONIC, &tw1);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts1);

  int num_arg = argc - 1, pipes[num_arg][2];
  unsigned long ramLimitPerChild = ramUsage30Percent / num_arg;
  int cpuLimit = 200000 / num_arg;
  char ramLimitStr[16];
  snprintf(ramLimitStr, sizeof(ramLimitStr), "%luM", ramLimitPerChild);

  for (int i = 0; i < num_arg; i++)
  {
    if (pipe(pipes[i]) == -1)
    {
      printf("An error ocurred with opening the pipe\n");
      perror("pipe");
      return 1;
    }
    pid_t pid = fork();

    if (pid == -1)
    {
      perror("An erro ocurred with fork\n");
      return 4;
    }

    if (pid == 0)
    {
      close(pipes[i][0]);
      childSum(i * i + 1, i * i + 2, i, argv[i + 1], pipes[i][1], cgroup_dir,
               ramLimitStr, cpuLimit);
      close(pipes[i][1]);
      return 0;
    }
    else if (pid < 0)
    {
      perror("error fork");
      return 1;
    }
  }
  int status;
  pid_t finished_pid;

  while ((finished_pid = wait(&status)) > 0)
  {
    printf("\n[Monitor] Child %d has finished. Capturing metrics...\n",
           finished_pid);

    char stats_path[200];
    snprintf(stats_path, sizeof(stats_path),
             "/sys/fs/cgroup/benchy/child-%d/memory.peak", finished_pid);

    FILE *f_mem = fopen(stats_path, "r");
    if (f_mem)
    {
      char peak_mem[32];
      if (fgets(peak_mem, sizeof(peak_mem), f_mem))
      {
        long long bytes = atoll(peak_mem);
        double mb = (double)bytes / (1024 * 1024);

        printf("Container %d - RAM Peak: %.2f MB\n", finished_pid, mb);
      }
      fclose(f_mem);
    }
    snprintf(stats_path, sizeof(stats_path),
             "/sys/fs/cgroup/benchy/child-%d/cpu.stat", finished_pid);
    FILE *f_cpu = fopen(stats_path, "r");
    if (f_cpu)
    {
      char line[100];
      while (fgets(line, sizeof(line), f_cpu))
      {
        if (strncmp(line, "usage_usec", 10) == 0)
        {
                      long long usec = atoll(line + 11); 
          double ms = (double)usec / 1000.0;
          printf("Container %d, CPU peak: %.2f ms\n", finished_pid, ms);
        }
      }
      fclose(f_cpu);
    }

    char cmd_cg[150];
    snprintf(cmd_cg, sizeof(cmd_cg), "/sys/fs/cgroup/benchy/child-%d",
             finished_pid);
    rmdir(cmd_cg);
  }
  clock_gettime(CLOCK_MONOTONIC, &tw2);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts2);

  time_duration =
      (ts2.tv_sec - ts1.tv_sec) * 1000.0 + (ts2.tv_nsec - ts1.tv_nsec) / 1e6;

  printf("CPU time used (per clock_gettime()): %.2f ms\n", time_duration);
  time_wall =
      (tw2.tv_sec - tw1.tv_sec) * 1000.0 + (tw2.tv_nsec - tw1.tv_nsec) / 1e6;

  printf("Wall time passed: %.2f ms\n", time_wall);
  for (int i = 0; i < num_arg; i++)
  {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "rm -rf /tmp/benchy/child-%d", pid_rm[i]);
    if (system(cmd) == -1)
    {
      perror("cmd error delete folder child");
    }
  }
  if (rmdir(cgroup_dir) == -1)
  {
    perror("rmdir");
    return 1;
  }
  return 0;
}
