#include <stdio.h>
#include "otg_common.c"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <endian.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>

#define SPLIT(X) ((X) >> 16), ((X) & 0xFFFF)
#define HEX32  "0x%04X_%04X"
#define HEX32_ "%04X_%04X"

static ssize_t _Write(uint8_t ch, void *arg) {
  *(*(uint8_t**)arg)++ = ch;
  return 1;
}

int _OpenImage(const char *fn, struct stat *st, void **pVirt) {
  int ec, fd = -1;
  void *virt = NULL;

  fd = open(fn, O_RDONLY|O_SYNC);
  if (fd < 0)
    return -1;

  ec = fstat(fd, st);
  if (ec < 0)
    goto fail;

  if (st->st_size < sizeof(ape_header))
    goto fail;

  virt = mmap(NULL, st->st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (!virt)
    goto fail;

  ape_header *hdr = virt;
  if (!memcmp("\x1AMCB", hdr->magic, 4) || !memcmp("\x1A" "BUB", hdr->magic, 4)) {
    fprintf(stderr, "error: file is word-swapped; un-word-swap the file in order to use this tool.\n");
    return 1;
  }

  if (memcmp("BCM\x1A", hdr->magic, 4) && memcmp("BUB\x1A", hdr->magic, 4)) {
    fprintf(stderr, "error: unrecognised magic\n");
    return 1;
  }

  *pVirt = virt;
  return fd;
fail:
  if (virt)
    munmap(virt, st->st_size);
  if (fd >= 0)
    close(fd);
  return -1;
}

int _CmdInfo(int pargc, int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: <ape.bin>\n");
    return 1;
  }

  bool errCRC = false;
  bool errCRCSwap = false;

  struct stat st;
  void *virt;
  int fd = _OpenImage(argv[1], &st, &virt);
  if (fd < 0) {
    fprintf(stderr, "error: cannot open file\n");
    return 1;
  }

  ape_header *hdr = virt;
  printf("---------- APE CODE IMAGE ----------\n");
  printf("Magic:              \"%s\"\n", hdr->magic);
  printf("Unk 04h:            " HEX32 "\n", SPLIT(le32toh(hdr->unk04)));
  printf("Image Name:         \"%s\"\n", hdr->imageName);
  printf("Image Version:      " HEX32 "\n", SPLIT(le32toh(hdr->imageVersion)));
  printf("Entrypoint:         " HEX32 "\n", SPLIT(le32toh(hdr->entrypoint)));
  printf("Unk 20h:            0x%02X\n", hdr->unk020);
  printf("Header Size:        0x%X bytes\n", hdr->headerSize*4);
  printf("Unk 22h:            0x%02X\n", hdr->unk022);
  printf("Num Sections:       %u\n", hdr->numSections);
  uint32_t hChecksumEx = le32toh(hdr->headerChecksum);
  uint32_t hwm = hdr->headerSize*4;

  uint32_t headerSize = le32toh(hdr->headerSize)*4;
  if (headerSize > st.st_size) {
    printf("error: short file\n");
    return 1;
  }

  ape_header *hdr2 = malloc(headerSize);
  assert(hdr2);
  memcpy(hdr2, hdr, headerSize);

  const char *str;
  char strbuf[64];

  hdr2->headerChecksum = 0;
  uint32_t hChecksumAc = ComputeCRC(hdr2, headerSize/4, 0);
  if (hChecksumEx == hChecksumAc)
    str = "OK";
  else if (!hChecksumEx) {
    snprintf(strbuf, sizeof(strbuf), "SKIPPED, checksum field zero");
    str = strbuf;
  } else {
    snprintf(strbuf, sizeof(strbuf), "MISMATCH, got " HEX32, SPLIT(hChecksumAc));
    str = strbuf;
    errCRC = true;
  }

  free(hdr2);

  printf("Header Checksum:    " HEX32 " %s\n", SPLIT(hChecksumEx), str);

  uint32_t trChecksumEx = le32toh(*(uint32_t*)((uint8_t*)virt + st.st_size - 4));
  uint32_t trChecksumAc = ComputeCRC(virt, (st.st_size-4)/4, 0xFFFFFFFF) ^ 0xFFFFFFFF;
  if (trChecksumEx == trChecksumAc)
    str = "OK";
  else {
    uint32_t *buf = malloc(st.st_size-4);
    for (size_t i=0; i<(st.st_size-4)/4; ++i)
      buf[i] = SwapEndian32(((uint32_t*)virt)[i]);

    trChecksumEx = SwapEndian32(trChecksumEx);
    trChecksumAc = ComputeCRC(buf, (st.st_size-4)/4, 0xFFFFFFFF) ^ 0xFFFFFFFF;
    if (trChecksumEx == trChecksumAc) {
      str = "SWAPPED";
      errCRCSwap = true;
    } else {
      snprintf(strbuf, sizeof(strbuf), "MISMATCH, got " HEX32, SPLIT(trChecksumAc));
      str = strbuf;
      errCRC = true;
    }

    free(buf);
  }

  printf("Trailing Checksum:  " HEX32 " %s\n", SPLIT(trChecksumEx), str);
  printf("File Size:          " HEX32 " bytes\n", SPLIT(st.st_size));

  printf("\n");
  printf("    Load Addr  Offset     Uncomp Sz  Comp Sz              Cksum    \n");
  printf("    ---------  ---------  ---------  ---------            ---------\n");
  for (size_t i=0; i<hdr->numSections; ++i) {
    uint32_t offsetFlags = le32toh(hdr->sections[i].offsetFlags);
    uint32_t offset = offsetFlags & 0xFFFFFF;
    uint32_t uncompressedSize = le32toh(hdr->sections[i].uncompressedSize);
    uint32_t compressedSize = le32toh(hdr->sections[i].compressedSize);
    uint32_t secChecksumEx = le32toh(hdr->sections[i].checksum);
    bool isZero = !!(offsetFlags & APE_SECTION_FLAG_ZERO_ON_FAST_BOOT);

    if (!isZero && offset + compressedSize > hwm)
      hwm = offset + compressedSize;

    str = "";
    if (!isZero) {
      void *compBuf = NULL;
      uint32_t compSize = 0;
      void *uncompBuf = NULL;
      uint32_t uncompSize = 0;

      compBuf  = (uint8_t*)virt + offset;
      compSize = (offsetFlags & APE_SECTION_FLAG_COMPRESSED) ? compressedSize : uncompressedSize;

      if (offset > st.st_size || compSize > st.st_size || (offset + compSize) > st.st_size) {
        compBuf = NULL;
        compSize = 0;
        str = "BAD LENGTH";
        errCRC = true;
      }

      if (compBuf) {
        if (offsetFlags & APE_SECTION_FLAG_COMPRESSED) {
          uncompSize = uncompressedSize;
          uncompBuf = malloc(uncompSize);
          assert(uncompBuf);

          uint8_t *ptr = uncompBuf;
          size_t bytesRead = 0, bytesWritten = 0;
          Decompress(compBuf, compSize, uncompSize, _Write, &ptr, &bytesRead, &bytesWritten);
        } else {
          uncompBuf = compBuf;
          uncompSize = compSize;
        }
      }

      if (uncompBuf) {
        uint32_t secChecksumAc;
        if (offsetFlags & APE_SECTION_FLAG_CHECKSUM_IS_CRC32) {
          secChecksumAc = ComputeCRC(uncompBuf, uncompSize/4, 0);
        } else {
          secChecksumAc = 0;
          uint32_t *uncompBuf_ = uncompBuf;
          for (size_t i=0; i<uncompSize/4; ++i)
            secChecksumAc += le32toh(uncompBuf_[i]);
          secChecksumAc += secChecksumEx;
          secChecksumEx = 0;
        }

        if (secChecksumAc == secChecksumEx)
          str = "OK";
        else {
          snprintf(strbuf, sizeof(strbuf), "MISMATCH, got " HEX32, SPLIT(secChecksumAc));
          str = strbuf;
          errCRC = true;
        }
      }

      if (uncompBuf && uncompBuf != compBuf)
        free(uncompBuf);
    }

    printf("%u)  " HEX32_ "  " HEX32_ "  " HEX32_ "  " HEX32_ "  %s%s%s%s%s%s%s%s  " HEX32_ "  %s",
      i,
      SPLIT(le32toh(hdr->sections[i].loadAddr)),
      SPLIT(offset),
      SPLIT(uncompressedSize),
      SPLIT(le32toh(hdr->sections[i].compressedSize)),

      (offsetFlags & APE_SECTION_FLAG_CHECKSUM_IS_CRC32) ? "S" : "P",
      (offsetFlags & APE_SECTION_FLAG_COMPRESSED)        ? "C" : "U",
      (offsetFlags & APE_SECTION_FLAG_ZERO_ON_FAST_BOOT) ? "Z" : " ",
      (offsetFlags & (1U<<26)) ? "X" : " ",
      (offsetFlags & (1U<<27)) ? "Y" : " ",
      (offsetFlags & (1U<<29)) ? "Z" : " ",
      (offsetFlags & (1U<<30)) ? "W" : " ",
      (offsetFlags & (1U<<31)) ? "R" : " ",

      SPLIT(secChecksumEx),
      str
    );
    printf("\n");
  }

  printf("\n");
  printf("Legend:\n");
  printf("  S - Checksum is CRC32, P - Checksum is sum-to-zero\n");
  printf("  C - Compressed, U - Uncompressed\n");
  printf("  U - Zero on boot (no space allocated in file)\n");
  printf("  X, Y, Z, W, R - Unknown flag bits 26, 27, 29, 30, 31\n");
  printf("Typical section assignment: Sec0=Scratchcode, Sec1=Code, Sec2=Data, Sec3=BSS\n");
  printf("\n");
  printf("HWM:          " HEX32 "\n", SPLIT(hwm));
  uint32_t slack = st.st_size-4-hwm;
  printf("Slack room:   0x%X bytes  %s\n", slack, slack == 0x100 ? "OK" : "IRREGULAR");
  if (errCRC || errCRCSwap || slack != 0x100)
    printf("Defects:     %s%s%s\n", errCRC ? " crc" : "", errCRCSwap ? " crcswap" : "", slack != 0x100 ? " slack" : "");
  else
    printf("Defects:      none\n");
  return 0;
}

static ssize_t _WriteStdout(uint8_t ch, void *arg) {
  return fputc(ch, stdout) != ch ? 0 : 1;
}

static int _CmdExtract(int pargc, int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: <ape-image.bin> <section-number>\n");
    return 2;
  }

  size_t secno = strtoul(argv[2], NULL, 0);

  struct stat st;
  void *virt;
  int fd = _OpenImage(argv[1], &st, &virt);
  if (fd < 0)
    return 1;

  ape_header *hdr = virt;
  uint32_t hdrSize = le32toh(hdr->headerSize)*4;
  if (hdrSize > st.st_size || hdrSize < ((hdr->numSections-4)*sizeof(ape_section) + sizeof(ape_header))) {
    fprintf(stderr, "error: short file\n");
    return 1;
  }

  if (secno >= hdr->numSections) {
    fprintf(stderr, "error: nonexistent section\n");
    return 1;
  }

  ape_section *sec = &hdr->sections[secno];
  uint32_t offset = le32toh(sec->offsetFlags) & 0xFFFFFF;
  bool isCompressed = le32toh(sec->offsetFlags) & APE_SECTION_FLAG_COMPRESSED;
  bool isZero = le32toh(sec->offsetFlags) & APE_SECTION_FLAG_ZERO_ON_FAST_BOOT;

  uint32_t compSize = le32toh(sec->compressedSize);
  uint32_t uncompSize = le32toh(sec->uncompressedSize);

  if (isZero) {
    while (uncompSize--)
      fputc(0, stdout);
    return 0;
  }

  if (!isCompressed) {
    if (offset > st.st_size || uncompSize > st.st_size || offset + uncompSize > st.st_size) {
      fprintf(stderr, "bad section offset/length\n");
      return 1;
    }

    ssize_t wr = fwrite((uint8_t*)virt + offset, uncompSize, 1, stdout);
    if (wr < 1)
      return 1;

    return 0;
  }

  if (offset > st.st_size || compSize > st.st_size || offset + compSize > st.st_size) {
    fprintf(stderr, "bad section offset/length\n");
    return 1;
  }

  size_t bytesRead, bytesWritten;
  Decompress((uint8_t*)virt + offset, compSize, uncompSize, _WriteStdout, NULL, &bytesRead, &bytesWritten);
  return 0;
}

static const struct argp _argp = {
  .args_doc = "<command> [command-args...]",
  .doc = "APE image servicing tool.\vCommands:\n"
    "  info     show information about an APE image\n"
    "  extract  extract an APE section, decompressing it if necessary\n"
    ,
};

typedef struct {
  const char *name;
  int (*func)(int pargc, int argc, char **argv);
} command_def_t;

static const command_def_t _commands[] = {
  {
    .name = "info",
    .func = _CmdInfo,
  },
  {
    .name = "extract",
    .func = _CmdExtract,
  },
  {},
};

int main(int argc, char **argv) {
  int argidx;
  error_t argerr = argp_parse(&_argp, argc, argv, ARGP_IN_ORDER, &argidx, NULL);
  if (argerr)
    return 2;

  const command_def_t *cmd = _commands;
  for (; cmd->name; ++cmd)
    if (argv[argidx] && !strcmp(cmd->name, argv[argidx]))
      break;

  if (!cmd->name) {
    argp_help(&_argp, stderr, ARGP_HELP_STD_USAGE, argv[0]);
    return 2;
  }

  return cmd->func(argidx, argc-argidx, argv+argidx);
}
