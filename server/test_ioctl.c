#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

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

struct aesd_seekto out;

int main(void) {
  out.write_cmd = 0;
  out.write_cmd_offset = 0;
  printf("Opening file\n");
  int fd = open("/dev/aesdchar", O_WRONLY);
  if (fd == -1) {
    perror("Cannot open file");
    exit(1);
  }
  printf("Test ioctl\n");

  ioctl(fd, 0x16, &out);

  printf("Test ioctl DONE!\n");

  return 0;
}
