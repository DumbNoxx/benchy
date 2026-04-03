#include "childs/sum.h"
#include "utils/utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
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
  pid_t childs_pids[argc - 1];
  double time_wall, resultBenchkCpu[argc - 1], resultBenchkRam[argc - 1];
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
      childSum(argv[i + 1], pipes[i][1], cgroup_dir, ramLimitStr, cpuLimit);
      close(pipes[i][1]);
      return 0;
    }
    if (pid != 0)
    {
      childs_pids[i] = pid;
      close(pipes[i][1]);

      if (read(pipes[i][0], &resultBenchkCpu[i], sizeof(double)) == -1)
      {
        perror("read pipe child");
        return 1;
      }
      if (read(pipes[i][0], &resultBenchkRam[i], sizeof(double)) == -1)
      {
        perror("read pipe child");
        return 1;
      }
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
    int idx = -1;
    for (int k = 0; k < num_arg; k++)
    {
      if (childs_pids[k] == finished_pid)
      {
        idx = k;
        break;
      }
    }
    printf("\n[Monitor] Child %d has finished. Capturing metrics...\n",
           finished_pid);
    printf("[Monitor] Testing binary: %s\n\n", argv[idx + 1]);
    printf("Container %d - CPU time bin: %.2fms\n", finished_pid,
           resultBenchkCpu[idx]);
    printf("Container %d - RAM usage bin: %.2fMB\n", finished_pid,
           resultBenchkRam[idx]);

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
          printf("Container %d, CPU Total peak: %.2f ms\n", finished_pid, ms);
        }
      }
      fclose(f_cpu);
    }
    int fail = 0;
    char procPath[128], sysPath[128], devPath[128];
    snprintf(procPath, sizeof(procPath), "/tmp/benchy/child-%d/proc",
             finished_pid);
    if (safe_umount(procPath) == -1)
      fail = 1;
    snprintf(sysPath, sizeof(sysPath), "/tmp/benchy/child-%d/sys",
             finished_pid);

    if (safe_umount(sysPath) == -1)
      fail = 1;
    snprintf(devPath, sizeof(devPath), "/tmp/benchy/child-%d/dev",
             finished_pid);

    if (safe_umount(devPath) == -1)
      fail = 1;
    if (fail == 0)
    {
      char cmd[128];
      snprintf(cmd, sizeof(cmd), "rm -rf /tmp/benchy/child-%d", finished_pid);
      if (system(cmd) == -1)
      {
        perror("cmd error delete folder child");
      }
    }
    else
    {
      perror("Error failed umount");
    }

    char cmd_cg[150];
    snprintf(cmd_cg, sizeof(cmd_cg), "/sys/fs/cgroup/benchy/child-%d",
             finished_pid);
    rmdir(cmd_cg);
  }
  clock_gettime(CLOCK_MONOTONIC, &tw2);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts2);

  time_wall =
      (tw2.tv_sec - tw1.tv_sec) * 1000.0 + (tw2.tv_nsec - tw1.tv_nsec) / 1e6;

  printf("\nWall time passed: %.2f ms\n", time_wall);
  if (rmdir(cgroup_dir) == -1)
  {
    perror("rmdir");
    return 1;
  }
  return 0;
}
