#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int openBin(char *arg, char *file, char *foundPath);

void childSum(char *arg, int fd, const char *cgroup_dir, char *ramLimit,
              int numCpu)
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
  char binaryPath[512];
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
  if (openBin(arg, file, binaryPath) == -1)
  {
    perror("openBin error in childSum");
    exit(1);
  }

  char sysPath[512], devPath[512], procPath[512];
  snprintf(sysPath, sizeof(sysPath), "%s/sys", file);
  snprintf(devPath, sizeof(devPath), "%s/dev", file);
  snprintf(procPath, sizeof(procPath), "%s/proc", file);

  if (mkdir(sysPath, 0555) == -1)
  {
    perror("mkdir syspath");
    exit(1);
  }
  if (mkdir(devPath, 0555) == -1)
  {
    perror("mkdir devPath");
    exit(1);
  }
  if (mkdir(procPath, 0555) == -1)
  {
    perror("mkdir /proc");
    exit(1);
  }

  mount("sysfs", sysPath, "sysfs", 0, NULL);
  mount("tmpfs", devPath, "tmpfs", 0, "size=1M");
  mount("proc", procPath, "proc", 0, NULL);

      if (unshare(CLONE_NEWNS | CLONE_NEWNET | CLONE_NEWPID) == -1)
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
  int silence_pipe[2];
  if (pipe(silence_pipe) == -1)
  {
    perror("pipe");
    exit(1);
  }

  pid_t n_pid = fork();
  if (n_pid == 0)
  {
    int memfd = memfd_create("benchmark_out", MFD_CLOEXEC);
    if (memfd != -1)
    {

      dup2(memfd, STDOUT_FILENO);
      dup2(memfd, STDERR_FILENO);
      close(memfd);
    }

    char *const args[] = {arg, NULL};
    if (execv(binaryPath, args) == -1)
    {
      perror("execv binary");
      exit(1);
    }

    exit(0);
  }
  else
  {
    close(silence_pipe[1]);
    close(silence_pipe[0]);
    struct rusage usage;
    wait4(n_pid, NULL, 0, &usage);
    double maxRssMb = (double)usage.ru_maxrss / 1024.0;

    double h_duration =
        (usage.ru_utime.tv_sec * 1000.0 + usage.ru_utime.tv_usec / 1000.0) +
        (usage.ru_stime.tv_sec * 1000.0 + usage.ru_stime.tv_usec / 1000.0);

    if (write(fd, &h_duration, sizeof(double)) == -1)
    {
      perror("error pipe h_duration");
      exit(1);
    }
    if (write(fd, &maxRssMb, sizeof(double)) == -1)
    {
      perror("error pipe maxRssMb");
      exit(1);
    }
  }
}

int openBin(char *arg, char *file, char *foundPath)
{
  char path[512] = {0};
  char command[512];
  snprintf(command, sizeof(command), "command -v %s", arg);

  FILE *fp = popen(command, "r");
  if (fp == NULL)
  {
    perror("popen openBin");
    return -1;
  }

  if (fgets(path, sizeof(path), fp) == NULL)
  {
    pclose(fp);
    char commandFallback[1024];
    snprintf(commandFallback, sizeof(commandFallback),
             "sudo -u $SUDO_USER env PATH=\"$PATH\" which %s 2>/dev/null", arg);
    FILE *fp2 = popen(commandFallback, "r");
    if (fp2 == NULL || fgets(path, sizeof(path), fp2) == NULL)
    {
      if (fp2)
        pclose(fp2);
      fprintf(stderr, "Error: Binary '%s' not found in PATH\n", arg);
      return -1;
    }
    pclose(fp2);
  }
  else
  {
    pclose(fp);
  }

  path[strcspn(path, "\n")] = 0;
  strcpy(foundPath, path);

  char libs[8192];
  char commandLib[1024];
  snprintf(commandLib, sizeof(commandLib), "ldd %s 2>&1", path);

  FILE *fpLib = popen(commandLib, "r");
  if (fpLib == NULL)
  {
    perror("open fpLib openBin");
    return -1;
  }

  libs[0] = '\0';
  char buffer[256];

  while (fgets(buffer, sizeof(buffer), fpLib) != NULL)
  {
    strncat(libs, buffer, sizeof(libs) - strlen(libs) - 1);
  }

  pclose(fpLib);

  if (strstr(libs, "not a dynamic executable") == NULL)
  {
    char copyLibs[2048];
    snprintf(copyLibs, sizeof(copyLibs),
             "echo '%s' | grep -o '/[a-zA-Z0-9._/-]*' | grep -v 'vdso' | xargs "
             "-I {} cp --parents -n {} %s/",
             libs, file);
    if (system(copyLibs) == -1)
    {
      perror("error copy libs to container");
      exit(1);
    }
  }
  else if (strlen(libs) == 0)
  {
    printf("Error get information libs");
  }

  char copyCmd[1024];
  snprintf(copyCmd, sizeof(copyCmd), "cp --parents %s %s/", path, file);

  FILE *fpCopy = popen(copyCmd, "r");
  int status = pclose(fpCopy);

  if (status != 0)
  {
    perror("error copy to container");
    return -1;
  }
  return 0;
}
