#define _GNU_SOURCE
#include "utils/utils.h"
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void childSum(int a, int b, int i, char *arg, int fd, const char *cgroup_dir,
              char *ramLimit, int numCpu)
{
  int pid = getpid();
  char child_path[128];
  snprintf(child_path, sizeof(child_path), "%s/child-%d", cgroup_dir, pid);
  if (mkdir(child_path, 0755) == -1)
  {
    perror("mkdir sub-cgroug");
    exit(1);
  }

  char memMaxPath[150];
  snprintf(memMaxPath, sizeof(memMaxPath), "%s/memory.max", child_path);
  int fdMem = open(memMaxPath, O_WRONLY);
  if (write(fdMem, ramLimit, strlen(ramLimit)) == -1)
  {
    perror("write ramLimit childSum");
    exit(1);
  }
  close(fdMem);
  char cpuLimit[50];
  snprintf(cpuLimit, sizeof(cpuLimit), "%d 100000", numCpu);
  char cpuMaxPath[150];
  snprintf(cpuMaxPath, sizeof(cpuMaxPath), "%s/cpu.max", child_path);
  int fdCpu = open(cpuMaxPath, O_WRONLY);
  if (fdCpu != -1)
  {
    if (write(fdCpu, cpuLimit, strlen(cpuLimit)) == -1)
    {
      perror("write cpu limit chidSum");
      exit(1);
    }
    close(fdCpu);
  }

  char proces_path[150];
  char pidResource[10];
  snprintf(proces_path, sizeof(proces_path), "%s/cgroup.procs", child_path);
  int fdProcs = open(proces_path, O_WRONLY);
  snprintf(pidResource, sizeof(pidResource), "%d", getpid());
  if (write(fdProcs, pidResource, strlen(pidResource)) == -1)
  {
    perror("write asdf");
    exit(1);
  }
  close(fdProcs);
  char file[50];
  sprintf(file, "/tmp/benchy/child-%d", pid);
  if (mkdir(file, 0777) == -1)
  {
    perror("mkdir");
    chmod(file, 0777);
  }

  if (unshare(CLONE_NEWNS | CLONE_NEWUSER | CLONE_NEWNET | CLONE_NEWPID) == -1)
  {
    perror("unshare");
    exit(1);
  }
  if (chroot(file) == -1)
  {
    perror("chroot");
    exit(1);
  }
  if (chdir("/") == -1)
  {
    perror("chdir");
    exit(1);
  }

  struct timespec h_ts1, h_ts2;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &h_ts1);
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

  unsigned long result = suma(a * i, b * a + i);
  ;

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
