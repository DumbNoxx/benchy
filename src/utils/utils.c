#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

int suma(int a, int b) { return a + b; }

int prepareTmp(const char *cgroup_dir)
{
  char clean_cmd[128];
  snprintf(clean_cmd, sizeof(clean_cmd), "rmdir %s/child-* 2>/dev/null",
           cgroup_dir);
  if (system(clean_cmd) == -1)
  {
    perror("clean_cmd");
  }

  if (rmdir(cgroup_dir) == -1 && errno != ENOENT)
  {
    perror("No se pudo limpiar el cgroup (¿hay procesos vivos?)");
  }
  if (mkdir("/tmp/benchy", 0777) == 0)
  {
    printf("Created folder in /tmp/benchy\n");
  }
  struct sysinfo si;

  if (sysinfo(&si) == -1)
  {
    perror("sysinfo");
    exit(1);
  }

  unsigned long total_ram = si.totalram * si.mem_unit / (1024 * 1024);
  int ramUsage30Percent = total_ram * 30 / 100;

  if (mkdir(cgroup_dir, 0755) == -1)
  {
    perror("mkdir /sys/fs/cgroup");
    exit(1);
  }
  char control_path[150];

  snprintf(control_path, sizeof(control_path), "%s/cgroup.subtree_control",
           cgroup_dir);
  int fd_ctrl = open(control_path, O_WRONLY);
  if (fd_ctrl != -1)
  {
    if (write(fd_ctrl, "+memory +cpu", 12) == -1)
    {
      perror("write preateTpm");
    }
    close(fd_ctrl);
  }
  else
  {
    perror("Error habilitando subtree_control");
  }

  char memoryMaxPath[100];
  snprintf(memoryMaxPath, sizeof(memoryMaxPath), "%s/memory.max", cgroup_dir);
  int fd = open(memoryMaxPath, O_WRONLY);
  char totalRamUsage[40];
  sprintf(totalRamUsage, "%dM", ramUsage30Percent);
  if (write(fd, totalRamUsage, strlen(totalRamUsage)) == -1)
  {
    perror("write");
    exit(1);
  }
  close(fd);

  sleep(2);
  return ramUsage30Percent;
}
int safe_umount(const char *path)
{
  if (umount(path) == -1)
  {
    if (umount2(path, MNT_DETACH) == -1)
    {
      perror("Error fatal al desmontar");
      return -1;
    }
    return -1;
  }
  return 0;
}
