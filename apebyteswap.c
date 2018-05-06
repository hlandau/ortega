#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include "otg_common.c"

int main(int argc, char **argv) {
  int ec;

  if (argc < 2) {
    fprintf(stderr, "usage: <file-to-apebyteswap>\n");
    return 2;
  }

  int fd = open(argv[1], O_RDWR|O_SYNC);
  if (fd < 0)
    return 1;

  struct stat st;
  ec = fstat(fd, &st);
  if (ec < 0)
    return 1;

  void *virt = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (!virt)
    return 1;

  uint32_t *v32 = virt;
  uint32_t numWords = st.st_size/4;

  uint32_t oldCRC = ComputeCRC(virt, numWords-1, 0xFFFFFFFF)^0xFFFFFFFF;
  if (oldCRC != le32toh(v32[numWords-1])) {
    fprintf(stderr, "old trailing CRC is not valid\n");
    return 1;
  }

  for (size_t i=0; i<numWords-1; ++i)
    v32[i] = SwapEndian32(v32[i]);

  v32[numWords-1] = ComputeCRC(virt, numWords-1, 0xFFFFFFFF)^0xFFFFFFFF;

  ec = munmap(virt, st.st_size);
  if (ec < 0)
    return 1;

  ec = close(fd);
  if (ec < 0)
    return 1;

  return 0;
}
