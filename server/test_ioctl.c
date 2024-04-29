#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct aesd_seekto {
  /**
   * The zero referenced write command to seek into
   */
  uint32_t write_cmd;
  /**
   * The zero referenced offset within the write
   */
  uint32_t write_cmd_offset;
};

#define AESD_IOC_MAGIC 0x16

#define AESDCHAR_IOCSEEKTO _IOWR(AESD_IOC_MAGIC, 1, struct aesd_seekto)

struct aesd_seekto out;

int main(int argc, char *argv[]) {
  if (argc == 3) {
    out.write_cmd = atoi(argv[1]);
    out.write_cmd_offset = atoi(argv[2]);
  } else {
    out.write_cmd = 0;
    out.write_cmd_offset = 0;
  }
  printf("Test ioctl with write_cmd = %u and write_cmd_offset = %u\n",
         out.write_cmd, out.write_cmd_offset);

  printf("Opening file\n");
  int fd = open("/dev/aesdchar", O_WRONLY);
  if (fd == -1) {
    perror("Cannot open file");
    exit(1);
  }

  int ret = ioctl(fd, AESDCHAR_IOCSEEKTO, &out);
  printf("ioctl returned: %d\n", ret);

  printf("Test ioctl DONE!\n");
  close(fd);

  return 0;
}
