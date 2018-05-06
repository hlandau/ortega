#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <endian.h>
#include <fcntl.h>
#include <unistd.h>
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
    fprintf(stderr, "Stamps S1 image with CRC and performs sanity checks. Build system use only.\n");
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

  // There's a reason we do all these sanity tests - it's really easy to mess
  // up the linker script. Getting it working took a lot of fiddling and it
  // could easily be broken. The trouble is that a linker script doesn't afford
  // much control over how the sections are appended into the output file - the
  // sections are crudely concatenated. The linker expects that the sections
  // will be loaded at virtual addresses with proper alignment, etc., but that
  // doesn't necessarily correspond with the padding in the output file - after
  // all, binary output mode is a bit of a hack and usually you'd have section
  // headers specifying what alignment to load everything with, etc. Moreover,
  // lld's support for GNU ld linker scripts leaves a lot to be desired.
  //
  // The consequence of this is that it's quite easy to mess things up in a way
  // that the linker believes the virtual address of any given resource in
  // the program image (function, etc.) to be different to the one which the
  // actual ROM loader will load that resource at.
  //
  // Here's the real example which motivated the following tests: The
  // otg_header is 0x28C bytes long. The s1Size field of otg_header is set to
  // 0x28C, indicating that the first opcode of the entrypoint starts 0x28C
  // beyond the start of the header. In the original firmware, the first opcode
  // indeed appears at this location.
  //
  // However, getting the first opcode to appear at this location with lld
  // turned out to be infeasible, because of its insistence on proper padding
  // of sections. It wanted to align .text to 16 bytes, which meant padding the
  // header to 0x290 and putting the first opcode there. Even where lld could
  // be convinced to put the first opcode at 0x28C, it would believe that the
  // ultimate virtual address of .text would be four bytes later, causing an
  // off-by-one-word error in all absolute jumps.
  //
  // To summarise:
  //   - A zero word is a NOP in MIPS;
  //   - Thus putting a zero word at 0x28C and having the entrypoint jump at
  //     0x290 is harmless;
  //   - It's easy to mess up the linker script in a way that causes absolute
  //     jumps to be off by one or two words;
  //   - We really don't want such a bug to be introduced accidentially, so
  //     this check ensures that the first jump lines up properly and that the
  //     first opcodes haven't moved around unexpectedly in the output file.
  if (L32(0x00) != HEADER_MAGIC) {
    fprintf(stderr, "S1 has bad magic\n");
    return 1;
  }

  // We always load at 0x08003800.
  uint32_t imageBase = L32(0x04);
  if (imageBase != 0x08003800) {
    fprintf(stderr, "S1 has wrong image base\n");
    return 1;
  }

  // The header should not be undersized, nor should any header padding
  // be excessive.
  uint32_t s1Size = L32(0x08)*4;
  uint32_t s1Offset = L32(0x0C);
  if (s1Offset < sizeof(otg_header) || s1Offset > (sizeof(otg_header)+16)) {
    fprintf(stderr, "S1 has wrong offset\n");
    return 1;
  }

  // We must find an opcode matching our expectations at @s1Offset. Note that
  // we do not allow *any* variation in this opcode - it is an absolute jump to
  // another, very close instruction which appears in the same block of
  // assembly, and it should always appear at the very start of the image
  // loaded into memory (which doesn't include the header and its padding), so
  // there should be absolutely zero variation in this opcode, and if there is,
  // something is wrong.
  //
  // Since this is an absolute jump, checking this allows us to ensure that the
  // linker has the right idea of where the code is loaded in memory,
  // preventing any off-by-one-word virtual address errors from going
  // unnoticed.
  uint32_t firstOpcode = L32(s1Offset);
  if (firstOpcode != 0x0A000E03) {
    fprintf(stderr, "First opcode is not expected value, got 0x%08X\n", firstOpcode);
    return 1;
  }

  // Sanity check, the branch delay slot must be a NOP.
  uint32_t secondOpcode = L32(s1Offset+4);
  if (secondOpcode) {
    fprintf(stderr, "Second opcode is not NOP, got 0x%08X\n", secondOpcode);
    return 1;
  }

  // Beyond the first jump and the NOP in its branch delay slot comes a pointer
  // to a version string as a virtual address. First, check that this is in
  // the right ballpark.
  uint32_t vstrPtr = L32(s1Offset+0x08);
  if ((vstrPtr & 0xFFFF0000) != 0x08000000) {
    fprintf(stderr, "Unexpected version string pointer value\n");
    return 1;
  }

  // Check that the vstr is correct. This allows us to ensure that virtual
  // addresses are still correct even for resources at the end of the S1 image,
  // which ensures no anomalous padding in the output file has been output that
  // causes a mismatch between virtual addresses and file offsets.
  uint8_t *vstr = ((uint8_t*)g_virt) + (vstrPtr - imageBase + s1Offset);

  bool isDummy = false;
  if (vstr[0] == 'D' && vstr[1] == 'u' && vstr[2] == 'm' && vstr[3] == 'm' && vstr[4] == 'y')
    isDummy = true;
  else if (vstr[0] != 'O' || vstr[1] != 'T' || vstr[2] != 'G') {
    fprintf(stderr, "Bad version string\n");
    return 1;
  }

  // Check that the stack pointer load opcodes are correct. We always set the
  // stack end to to 0x0800_7000, so this should never change either.
  uint32_t thirdOpcode = L32(s1Offset+0x0C);
  uint32_t fourthOpcode = L32(s1Offset+0x10);
  if (thirdOpcode != 0x3C1D0800 || fourthOpcode != 0x27BD7000) {
    fprintf(stderr, "Third/fourth opcode is not expected stack pointer load, got 0x%08X 0x%08X\n", thirdOpcode, fourthOpcode);
    return 1;
  }

  if (!isDummy) {
    // The S2 header should appear at the right location. Just check for the
    // magic; the S2 image is included as a fixed binary, so we can do any S2
    // sanity checks in s2stamp. We just need to make sure it's at the right
    // offset.
    if (L32(s1Offset+s1Size) != HEADER_MAGIC) {
      fprintf(stderr, "S2 not found at expected offset\n");
      return 1;
    }
  }

  // Finally we can start setting CRCs.
  if (L32(16) != 0xDEADBEEF) {
    fprintf(stderr, "Boot header CRC placeholder not found.\n");
    return 1;
  }

  // Stage 1 Image CRC.
  if (L32(s1Offset+s1Size-4) != 0xDEADBEEF) {
    fprintf(stderr, "S1 image CRC placeholder not found.\n");
    return 1;
  }

  uint32_t crc;
  ssize_t wr;

  // Boot header CRC.
  crc = htole32(ComputeCRC(virt, 16/4, 0xFFFFFFFF)^0xFFFFFFFF);
  wr = pwrite(fd, &crc, sizeof(crc), 0x10);
  if (wr < sizeof(crc))
    return 1;

  // Directory CRC.
  uint8_t dirSum = 0;
  uint8_t *dirEnd = (uint8_t*)virt + 0x74;
  for (uint8_t *p = (uint8_t*)virt + 0x14; p < dirEnd; ++p)
    dirSum += *p;

  uint8_t dirCRC = 0 - dirSum;
  wr = pwrite(fd, &dirCRC, sizeof(dirCRC), 0x75);
  if (wr < sizeof(dirCRC))
    return 1;

  // Manufacturing Section CRC.
  crc = htole32(ComputeCRC((uint8_t*)virt + 0x74, 0x88/4, 0xFFFFFFFF)^0xFFFFFFFF);
  wr = pwrite(fd, &crc, sizeof(crc), 0xFC);
  if (wr < sizeof(crc))
    return 1;

  // Manufacturing Section 2 CRC.
  crc = htole32(ComputeCRC((uint8_t*)virt + 0x200, 0x88/4, 0xFFFFFFFF)^0xFFFFFFFF);
  wr = pwrite(fd, &crc, sizeof(crc), 0x288);
  if (wr < sizeof(crc))
    return 1;

  // S1 Image CRC.
  crc = htole32(ComputeCRC((uint8_t*)virt + s1Offset, (s1Size/4)-1, 0xFFFFFFFF)^0xFFFFFFFF);
  wr = pwrite(fd, &crc, sizeof(crc), s1Offset+s1Size-4);
  if (wr < sizeof(crc))
    return 1;

  close(fd);
  return 0;
}
