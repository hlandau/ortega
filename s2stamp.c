#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <endian.h>
#include "otg.h"
#include "otg_common.c"

static void *g_virt;

static uint32_t L32(uint32_t offset) {
  return ntohl(*(uint32_t*)(((uint8_t*)g_virt) + offset));
}

int main(int argc, char **argv) {
  int ec;

  if (argc < 2) {
    fprintf(stderr, "usage: <image-file>\n");
    fprintf(stderr, "Stamps S2 image with CRC and performs sanity checks. Build system use only.\n");
    return 2;
  }

  int fd = open(argv[1], O_RDWR);
  if (fd < 0)
    return 1;

  struct stat st;
  ec = fstat(fd, &st);
  if (ec < 0)
    return 1;

  void *virt = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (!virt)
    return 1;

  if (st.st_size % 4) {
    fprintf(stderr, "error: filesize not a multiple of 4\n");
    return 1;
  }

  g_virt = virt;

  // See s1stamp.c for an explanation of the motivations behind
  // these tests.
  if (L32(0x00) != HEADER_MAGIC) {
    fprintf(stderr, "S2 has bad magic\n");
    return 1;
  }

  uint32_t s2Size = L32(0x04);

  uint32_t firstOpcode = L32(0x08);
  if (firstOpcode) { // != 0x0A000005) {
    fprintf(stderr, "First opcode is not expected value, got 0x%08X\n", firstOpcode);
    return 1;
  }

  uint32_t secondOpcode = L32(0x0C);
  if (secondOpcode != 0x10000004) {
    fprintf(stderr, "Second opcode is not expected value, got 0x%08X\n", secondOpcode);
    return 1;
  }

  uint32_t thirdOpcode = L32(0x10);
  if (thirdOpcode) {
    fprintf(stderr, "Third opcode is not expected NOP, got 0x%08X\n", thirdOpcode);
    return 1;
  }

  uint32_t fourthOpcode = L32(0x20);
  uint32_t fifthOpcode = L32(0x24);
  if (fourthOpcode != 0x3C1D0800 || fifthOpcode != 0x27BD7000) {
    fprintf(stderr, "Fourth/fifth opcode is not expected stack pointer load, got 0x%08X 0x%08X\n", fourthOpcode, fifthOpcode);
    return 1;
  }

  if (L32(s2Size+8-4) != 0xDEADBEEF) {
    fprintf(stderr, "S2 CRC placeholder not found.\n");
    return 1;
  }

  // Write the correct CRC.
  uint32_t crc = ComputeCRC((uint8_t*)virt+8, (st.st_size-4-8)/4, 0xFFFFFFFF);
  crc = htole32(crc^0xFFFFFFFF);

  ssize_t wr = pwrite(fd, &crc, sizeof(crc), st.st_size-4);
  if (wr < sizeof(crc))
    return 1;

  close(fd);
  return 0;
}
