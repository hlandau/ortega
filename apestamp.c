#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>
#include <endian.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "otg.h"
#include "otg_common.c"

#define BCM1A_MAGIC 0x1A4D4342 /* "BCM\x1A" */

static const void *g_virt;

static uint32_t L32(uint32_t offset) {
  return le32toh(*(uint32_t*)(((uint8_t*)g_virt) + offset));
}

#define N 2048
#define F 34
#define THRESHOLD 2
#define NIL N

typedef struct {
  uint8_t dict[N+F-1];

  // Describes longest match. Set by _InsertNode.
  int matchPos, matchLen;
  // Left and right children and parents. Makes up a binary search tree.
  int lson[N+1], rson[N+257], parent[N+1];
} compressor_state;

// Inserts a string of length F, text_buf[r..r+F-1] into one of the trees
// (dict[r]'th tree) and returns the longest-match position and length via the
// state variables matchPosition and matchLength. If matchLength == F, then
// removes the old node in favour of the new one, because the old one will be
// deleted sooner. Note that r plays a double role, as the tree node index and
// the position in the buffer.
static void _InsertNode(compressor_state *st, int r) {
  int p, cmp;
  uint8_t *key;

  uint8_t *dict = st->dict;
  int *lson = st->lson, *rson = st->rson, *parent = st->parent;

  cmp = 1;
  key = &dict[r];
  p = N+1+key[0];
  rson[r] = lson[r] = NIL;
  st->matchLen = 0;
  for (;;) {
    if (cmp >= 0) {
      if (rson[p] != NIL)
        p = rson[p];
      else {
        rson[p] = r;
        parent[r] = p;
        return;
      }
    } else {
      if (lson[p] != NIL)
        p = lson[p];
      else {
        lson[p] = r;
        parent[r] = p;
        return;
      }
    }

    // Compare.
    int i;
    for (i=1; i<F; ++i) {
      cmp = key[i] - dict[p+i];
      if (cmp)
        break;
    }
    if (i > st->matchLen) {
      // We have found a longer match.
      st->matchPos = p;
      st->matchLen = i;
      if (i >= F)
        // Maximum match length, stop looking.
        break;
    }
  }

  parent[r]  = parent[p];
  lson[r] = lson[p];
  rson[r] = rson[p];
  parent[lson[p]] = r;
  parent[rson[p]] = r;
  if (rson[parent[p]] == p)
    rson[parent[p]] = r;
  else
    lson[parent[p]] = r;
  parent[p] = NIL;
}

// Deletes node p from the tree.
static void _DeleteNode(compressor_state *st, int p) {
  int q;

  int *lson = st->lson, *rson = st->rson, *parent = st->parent;

  if (parent[p] == NIL)
    // Not in tree.
    return;

  if (rson[p] == NIL)
    q = lson[p];
  else if (lson[p] == NIL)
    q = rson[p];
  else {
    q = lson[p];
    if (rson[q] != NIL) {
      do
        q = rson[q];
      while (rson[q] != NIL);
      rson[parent[q]] = lson[q];
      parent[lson[q]] = parent[q];
      lson[q] = lson[p];
      parent[lson[p]] = q;
    }

    rson[q] = rson[p];
    parent[rson[p]] = q;
  }

  parent[q] = parent[p];
  if (rson[parent[p]] == p)
    rson[parent[p]] = q;
  else
    lson[parent[p]] = q;
  parent[p] = NIL;
}

// Compression routine adapted from original 1989 LZSS.C by Haruhiko Okumura.
// "Use, distribute, and modify this program freely."
static void _Compress(const void *in, size_t inBytes, FILE *fo, size_t *bytesRead, size_t *bytesWritten) {
  const uint8_t *in_ = in;
  const uint8_t *inEnd = in_ + inBytes;
  size_t bytesWritten_ = 0;

  compressor_state st;

  if (!inBytes)
    return;

  // Initialize tree.
  for (int i=N+1; i <= N+256; ++i)
    st.rson[i] = NIL;
  for (int i=0; i<N; ++i)
    st.parent[i] = NIL;

  int i, c, len, r = N-F, s = 0, lastMatchLen, codeBufPtr = 1;
  uint8_t codeBuf[17], mask = 1;
  codeBuf[0] = 0;

  // Clear the buffer.
  for (i=0; i<r; ++i)
    st.dict[i] = 0x20;

  // Read F bytes into the last F bytes of the buffer.
  for (len=0; len < F && in_ < inEnd; ++len)
    st.dict[r+len] = c = *in_++;

  // Insert the F strings, each of which begins with one or more 'space'
  // characters. Note the order in which these strings are inserted. This way,
  // degenerate trees will be less likely to occur.
  for (i=1; i<=F; ++i)
    _InsertNode(&st, r-i);

  // Finally, insert the whole string just read. matchLength and matchPosition are set.
  _InsertNode(&st, r);

  do {
    // matchLen may be spuriously long near the end of text.
    if (st.matchLen >= len)
      st.matchLen = len;

    if (st.matchLen <= THRESHOLD) {
      // Not long enough match. Send one byte.
      st.matchLen = 1;
      codeBuf[0] |= mask; // "Send one byte" flag.
      codeBuf[codeBufPtr++] = st.dict[r]; // Send uncoded.
      //printf("  LIT 0x%02x\n", st.dict[r]);
    } else {
      // Send position and length pair. Note that matchLen > THRESHOLD.
      //printf("  REF off=%4u  len=%4u\n", st.matchPos, st.matchLen);
      //printf("    ");
      //for (size_t j=0; j<st.matchLen; ++j)
      //  printf("%02x ", st.dict[st.matchPos+j]);
      //printf("\n");
      codeBuf[codeBufPtr++] = (uint8_t)st.matchPos;
      assert(st.matchLen - (THRESHOLD+1) < 0x20);
      codeBuf[codeBufPtr++] = (uint8_t)(((st.matchPos >> 3) & 0xE0) | (st.matchLen - (THRESHOLD+1)));
    }

    mask <<= 1;
    if (!mask) {
      // Send at most eight units of code together.
      for (i=0; i<codeBufPtr; ++i)
        fputc(codeBuf[i], fo);
      bytesWritten_ += codeBufPtr;
      codeBuf[0] = 0;
      codeBufPtr = mask = 1;
    }

    lastMatchLen = st.matchLen;
    for (i=0; i<lastMatchLen && in_ < inEnd; ++i) {
      // Delete old strings and read new bytes.
      c = *in_++;
      _DeleteNode(&st, s);
      st.dict[s] = c;
      // If the position is near the end of the buffer, extend the buffer to
      // make string comparison easier.
      if (s < F-1)
        st.dict[s+N] = c;
      // Since this is a ring buffer, increment the position modulo N.
      s = (s+1) % N;
      r = (r+1) % N;
      // Register the string in dict[r..r+F-1].
      _InsertNode(&st, r);
    }

    while (i++ < lastMatchLen) {
      // After the end of text, no need to read, but buffer may not be empty.
      _DeleteNode(&st, s);
      s = (s+1) % N;
      r = (r+1) % N;
      if (--len)
        _InsertNode(&st, r);
    }
  } while (len > 0);

  // Send remaining code.
  if (codeBufPtr > 1) {
    for (i=0; i<codeBufPtr; ++i)
      fputc(codeBuf[i], fo);
    bytesWritten_ += codeBufPtr;
  }

  *bytesRead = in_ - (uint8_t*)in;
  *bytesWritten = bytesWritten_;
}

static ssize_t _WriteBuf(uint8_t ch, void *arg) {
  *(*(uint8_t**)arg)++ = ch;
  return 1;
}

int main(int argc, char **argv) {
  int ec;

  if (argc < 3) {
    fprintf(stderr, "usage: <input-image-file output-image-file>\n");
    fprintf(stderr, "Stamps APE code image with CRC, compresses sections, performs sanity checks. Build system use only.\n");
    return 2;
  }

  int fd = open(argv[1], O_RDONLY|O_SYNC);
  if (fd < 0)
    return 1;

  struct stat st;
  ec = fstat(fd, &st);
  if (ec < 0) {
    fprintf(stderr, "can't fstat\n");
    return 1;
  }

  const void *virt = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (!virt) {
    fprintf(stderr, "can't mmap\n");
    return 1;
  }

  if (st.st_size % 4) {
    fprintf(stderr, "error: filesize not a multiple of 4\n");
    return 1;
  }

  g_virt = virt;
  const ape_header *hdr = virt;

  if (memcmp(hdr->magic, "BCM\x1A", 4)) {
    fprintf(stderr, "image has bad magic\n");
    return 1;
  }

  if (hdr->numSections != 4) {
    fprintf(stderr, "unexpected number of sections\n");
    return 1;
  }

  if (hdr->headerSize*4 < (sizeof(ape_header) - sizeof(hdr->sections) + sizeof(ape_section)*hdr->numSections)) {
    fprintf(stderr, "short header\n");
    return 1;
  }

  if (le32toh(hdr->entrypoint) != 0x001080C0) {
    fprintf(stderr, "bad entrypoint\n");
    return 1;
  }

  if (le32toh(hdr->headerChecksum) != 0xDEADBEEF) {
    fprintf(stderr, "header CRC placeholder not found\n");
    return 1;
  }

  if (L32(st.st_size-4) != 0xDEADBEEF) {
    fprintf(stderr, "trailing CRC placeholder not found, 0x%08X\n", L32(st.st_size-4));
    return 1;
  }

  ape_header *hdr2 = malloc(hdr->headerSize*4);
  assert(hdr2);

  memcpy(hdr2, hdr, hdr->headerSize*4);

  for (size_t i=0; i<hdr2->numSections; ++i)
    if (le32toh(hdr2->sections[i].checksum) != 0xDEADBEEF
        && !(le32toh(hdr2->sections[i].offsetFlags & APE_SECTION_FLAG_ZERO_ON_FAST_BOOT))) {
      fprintf(stderr, "section CRC placeholder not found\n");
      return 1;
    }

  FILE *fo = fopen(argv[2], "w+b");
  if (!fo) {
    fprintf(stderr, "cannot open output file\n");
    return 1;
  }

  size_t bodyStart = hdr2->headerSize*4;
  ec = fseek(fo, bodyStart, SEEK_SET);
  if (ec < 0) {
    fprintf(stderr, "seek error?\n");
    return 1;
  }

  // Check sections.
  size_t curOffset = bodyStart;
  for (size_t i=0; i<hdr2->numSections; ++i) {
    bool isZero = (le32toh(hdr2->sections[i].offsetFlags) & APE_SECTION_FLAG_ZERO_ON_FAST_BOOT);
    bool isCompressed = (le32toh(hdr2->sections[i].offsetFlags) & APE_SECTION_FLAG_COMPRESSED);

    if (isZero) {
      if (isCompressed) {
        fprintf(stderr, "BSS section is compressed\n");
        return 1;
      }

      if (le32toh(hdr2->sections[i].compressedSize)) {
        fprintf(stderr, "BSS section with nonzero compressed size\n");
        return 1;
      }

      if (le32toh(hdr2->sections[i].checksum)) {
        fprintf(stderr, "BSS section with nonzero checksum\n");
        return 1;
      }

      if (le32toh(hdr2->sections[i].offsetFlags) & APE_SECTION_FLAG_CHECKSUM_IS_CRC32) {
        fprintf(stderr, "BSS section with CRC32 flag\n");
        return 1;
      }

      continue;
    }

    if (isCompressed) {
      fprintf(stderr, "section already compressed?\n");
      return 1;
    }

    if (le32toh(hdr2->sections[i].compressedSize)) {
      fprintf(stderr, "nonzero compressed size\n");
      return 1;
    }

    uint32_t offset = le32toh(hdr2->sections[i].offsetFlags) & 0xFFFFFF;
    if (offset != curOffset) {
      fprintf(stderr, "gap found\n");
      return 1;
    }

    uint32_t uncompSize = le32toh(hdr2->sections[i].uncompressedSize);
    if (uncompSize % 4) {
      fprintf(stderr, "section uncompressed size is not a multiple of 4 bytes\n");
      return 1;
    }

    if (uncompSize > st.st_size || (offset + uncompSize) > st.st_size) {
      fprintf(stderr, "section exceeds file length\n");
      return 1;
    }

    curOffset += uncompSize;
  }

  void *verifyInBuf = NULL, *verifyOutBuf = NULL;
  size_t verifyInBufSize = 0, verifyOutBufSize = 0;

  // Compress and output sections.
  for (size_t i=0; i<hdr2->numSections; ++i) {
    if (le32toh(hdr2->sections[i].offsetFlags) & APE_SECTION_FLAG_ZERO_ON_FAST_BOOT) {
      // Zero the offset for BSS sections, it's not used anyway.
      hdr2->sections[i].offsetFlags = htole32(le32toh(hdr2->sections[i].offsetFlags) & 0xFF000000);
      continue;
    }

    uint32_t uncompSize = le32toh(hdr->sections[i].uncompressedSize);

    long compStart = ftell(fo);
    if (compStart < 0) {
      fprintf(stderr, "compStart\n");
      return 1;
    }

    uint32_t offset = le32toh(hdr2->sections[i].offsetFlags) & 0xFFFFFF;
    hdr2->sections[i].offsetFlags = htole32(compStart
      | APE_SECTION_FLAG_COMPRESSED | APE_SECTION_FLAG_CHECKSUM_IS_CRC32 | (1U<<27) | (i<2 ? (1U<<26) : 0));
    size_t readBytes = 0;
    size_t writtenBytes = 0;
    _Compress(
      (uint8_t*)hdr + offset,
      uncompSize,
      fo, &readBytes, &writtenBytes);

    if (readBytes < uncompSize) {
      fprintf(stderr, "did not read all input bytes?\n");
      return 1;
    }

    size_t r = writtenBytes % 4;
    if (r) {
      static const uint8_t _zeroes[4] = {};
      ec = fwrite(_zeroes, 4-r, 1, fo);
      if (ec < 1) {
        fprintf(stderr, "fwrite\n");
        return 1;
      }
      writtenBytes += 4-r;
    }

    hdr2->sections[i].compressedSize = htole32(writtenBytes);

    long compEnd = ftell(fo);
    if (compEnd < 0)
      return 1;

    assert(compEnd - compStart == writtenBytes);

    ec = fseek(fo, compStart, SEEK_SET);
    if (ec < 0) {
      fprintf(stderr, "fseek\n");
      return 1;
    }

    if (compEnd - compStart > verifyInBufSize) {
      verifyInBufSize = compEnd - compStart;
      verifyInBuf = realloc(verifyInBuf, verifyInBufSize);
      assert(verifyInBuf);
    }

    if (uncompSize > verifyOutBufSize) {
      verifyOutBufSize = uncompSize;
      verifyOutBuf = realloc(verifyOutBuf, verifyOutBufSize);
      assert(verifyOutBuf);
    }

    ssize_t rd = fread(verifyInBuf, compEnd - compStart, 1, fo);
    if (compEnd != compStart && rd < 1) {
      fprintf(stderr, "fread verify %ld\n", rd);
      return 1;
    }

    ec = fseek(fo, compEnd, SEEK_SET);
    if (ec < 0) {
      fprintf(stderr, "compEnd\n");
      return 1;
    }

    uint8_t *p = verifyOutBuf;
    size_t verifyRead, verifyWritten;
    Decompress(verifyInBuf, compEnd - compStart, uncompSize, _WriteBuf, &p, &verifyRead, &verifyWritten);

    if (verifyWritten != uncompSize) {
      fprintf(stderr, "compression verification outputted wrong amount of data (got %u bytes, expected %u)\n", verifyWritten, uncompSize);
      return 1;
    }

    if (memcmp(verifyOutBuf, (uint8_t*)hdr + offset, uncompSize)) {
      fprintf(stderr, "compression verification failed\n");
      //fwrite((uint8_t*)hdr + offset, uncompSize, 1, stdout);
      //fwrite(verifyOutBuf, uncompSize, 1, stdout);
      return 1;
    }

    hdr2->sections[i].checksum = htole32(ComputeCRC((uint8_t*)hdr + offset, uncompSize/4, 0));
    hdr2->sections[i].compressedSize = compEnd - compStart;
  }

  ec = fseek(fo, 0, SEEK_SET);
  if (ec < 0) {
    fprintf(stderr, "fseek fo\n");
    return 1;
  }

  hdr2->headerChecksum = 0;
  hdr2->headerChecksum = htole32(ComputeCRC(hdr2, hdr2->headerSize, 0));

  ec = fwrite(hdr2, hdr2->headerSize*4, 1, fo);
  if (ec < 1)
    return 1;

  ec = fseek(fo, 0, SEEK_END);
  if (ec < 0)
    return 1;

  // Instead of an RSA signature, provide the user with a relaxing message
  char rsaBuf[256] = "OTG";
  static const char padStr[] = "DON'T PANIC! ";
  size_t padLen = strlen(padStr);
  for (int i=strlen(rsaBuf)+1; i < ARRAYLEN(rsaBuf); ++i)
    rsaBuf[i] = padStr[i % padLen];

  ssize_t wr = fwrite(rsaBuf, 256, 1, fo);
  if (wr < 1)
    return 1;

  ec = fflush(fo);
  if (ec < 0) {
    fprintf(stderr, "can't flush\n");
    return 1;
  }

  int fofd = fileno(fo);
  if (fofd < 0)
    return 1;

  ec = fstat(fofd, &st);
  if (ec < 0) {
    fprintf(stderr, "can't stat after write\n");
    return 1;
  }

  void *fovirt = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fofd, 0);
  if (!fovirt) {
    fprintf(stderr, "cannot mmap after write\n");
    return 1;
  }

  uint32_t trailingCRC = htole32(ComputeCRC(fovirt, st.st_size/4, 0xFFFFFFFF)^0xFFFFFFFF);
  wr = fwrite(&trailingCRC, 4, 1, fo);
  if (wr < 1)
    return 1;

  ec = fclose(fo);
  if (ec < 0)
    return 1;

  return 0;
}
