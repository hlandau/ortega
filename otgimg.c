#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <argp.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "otg.h"
#include "otg_common.c"

static const size_t HUMAN_SIZE_LEN = 64;

static char *_Humanize(uint32_t sz, char *buf) {
  const char *suffix = "B";
  if (sz >= 1024) {
    suffix = "KiB";
    sz /= 1024;
  }
  if (sz >= 1024) {
    suffix = "MiB";
    sz /= 1024;
  }
  snprintf(buf, HUMAN_SIZE_LEN, "%u %s", sz, suffix);
  return buf;
}

static const struct argp _argpInfo = {
  .doc = "Show information about a firmware image.\n",
  .args_doc = "<image-filename>",
};

static void _DumpVPDInner(const uint8_t *vpd, size_t vpdLen, size_t *ckOffsetPtr) {
  enum {
    VSTATE_KW1 = 0,
    VSTATE_KW2,
    VSTATE_LEN,
    VSTATE_DATA,
  };
  enum {
    VPD_KW_PART_NUMBER         = 0x504E, // 'PN'
    VPD_KW_ENGINEERING_CHANGES = 0x4543, // 'EC'
    VPD_KW_SERIAL_NUMBER       = 0x534E, // 'SN'
    VPD_KW_MANUFACTURE_ID      = 0x4D4E, // 'MN'
    VPD_KW_RV                  = 0x5256, // 'RV'
    VPD_KW_ASSET_TAG           = 0x5941, // 'YA'
    VPD_KW_READ_WRITE_AREA     = 0x5257, // 'RW'
    VPD_KW_V0                  = 0x5630, // 'V0' ] Unknown Vendor-Specific
    VPD_KW_V1                  = 0x5631, // 'V1' ]
    VPD_KW_V2                  = 0x5632, // 'V2' ]
    VPD_KW_V3                  = 0x5633, // 'V3' ]
    VPD_KW_V4                  = 0x5634, // 'V4' ]
    VPD_KW_V5                  = 0x5635, // 'V5' ]
    VPD_KW_V6                  = 0x5636, // 'V6' ]
  };
  uint16_t kw;
  uint8_t origLen, len;
  int state = VSTATE_KW1;
  const uint8_t *vpdEnd = vpd + vpdLen;
  char *buf = NULL;
  char *bufp = NULL;
  size_t bufLen = 0;
  const uint8_t *vpdStart = vpd;
  for (;vpd != vpdEnd || (state == VSTATE_DATA && !len); ++vpd) {
    switch (state) {
      case VSTATE_KW1:
        kw = *vpd;
        state = VSTATE_KW2;
        break;
      case VSTATE_KW2:
        kw = (kw<<8) | *vpd;
        state = VSTATE_LEN;
        break;
      case VSTATE_LEN:
        origLen = len = *vpd;
        state = VSTATE_DATA;
        bufp = buf;
        break;
      case VSTATE_DATA:
        if (len > bufLen) {
          bufLen = len;
          buf = bufp = realloc(buf, len+1);
          memset(buf, 0, len+1);
        }
        if (!len) {
          *bufp++ = 0;
          switch (kw) {
            case VPD_KW_PART_NUMBER:
              printf("    Part Number:         \"%s\"\n", buf);
              break;
            case VPD_KW_ENGINEERING_CHANGES:
              printf("    Engineering Changes: \"%s\"\n", buf);
              break;
            case VPD_KW_SERIAL_NUMBER:
              printf("    Serial Number:       \"%s\"\n", buf);
              break;
            case VPD_KW_MANUFACTURE_ID:
              printf("    Manufacture ID:      \"%s\"\n", buf);
              break;
            case VPD_KW_RV:
              printf("    (Checksum/End)\n");
              if (ckOffsetPtr)
                *ckOffsetPtr = (vpd - vpdStart);
              break;
            case VPD_KW_ASSET_TAG:
              printf("    Asset Tag:           \"%s\"\n", buf);
              break;
            case VPD_KW_READ_WRITE_AREA:
              printf("    (Read/Write Reserved Area)\n");
              break;
            case VPD_KW_V0:
            case VPD_KW_V1:
            case VPD_KW_V2:
            case VPD_KW_V3:
            case VPD_KW_V4:
            case VPD_KW_V5:
            case VPD_KW_V6:
              printf("    V%c:                  \"%s\"\n", kw & 0xFF, buf);
              break;
            default:
              printf("    Unknown VPD KW 0x%04X ('%c%c')\n", kw, kw>>8, kw & 0xFF);
              break;
          }
          state = VSTATE_KW1;
          --vpd;
          break;
        }

        *bufp++ = *vpd;
        --len;

        break;
      default:
        abort();
    }
  }
}

static int _DumpVPD(uint8_t *vpd, size_t vpdBufLen) {
  uint8_t *vpdEnd = vpd + vpdBufLen;
  enum {
    STATE_DRIFTING = 0,
    STATE_LEN1,
    STATE_LEN2,
    STATE_DATA,
  };
  enum {
    VPD_TYPE_IDENTIFIER_STRING = 0x02,
    VPD_TYPE_READ_ONLY         = 0x10,
    VPD_TYPE_READ_WRITE        = 0x11,
    VPD_TYPE_END               = 0x0F,
  };
  int state = STATE_DRIFTING;
  uint8_t vpdType;
  uint16_t vpdLen, vpdOrigLen;
  uint8_t *buf = NULL;
  size_t bufLen = 0;
  uint8_t *bufp = realloc(NULL, 1);
  size_t ckOffset = 0;
  uint8_t *vpdStart = vpd;
  bool errCRC = false;
  for (;vpd != vpdEnd || (state == STATE_DATA && !vpdLen); ++vpd) {
    switch (state) {
    case STATE_DRIFTING:
      if (*vpd & 0x80) {
        // Large tag
        vpdType = *vpd & 0x7F;
        state = STATE_LEN1;
      } else {
        // Small tag
        vpdType = ((*vpd) >> 3) & 0x0F;
        vpdLen  = (*vpd) & 0x07;
        state = STATE_DATA;
      }
      break;
    case STATE_LEN1:
      vpdLen = vpdOrigLen = *vpd;
      state = STATE_LEN2;
      bufp = buf;
      break;
    case STATE_LEN2:
      vpdLen |= ((uint32_t)(*vpd)) << 8;
      vpdOrigLen = vpdLen;
      state = STATE_DATA;
      bufp = buf;
      break;
    case STATE_DATA:
      if (vpdLen > bufLen) {
        bufLen = vpdLen;
        buf = bufp = realloc(buf, bufLen+1);
        memset(buf, 0, bufLen+1);
      }

      if (!vpdLen) {
        *bufp++ = 0;
        switch (vpdType) {
        case VPD_TYPE_IDENTIFIER_STRING:
          printf("  Identifier: \"%s\"\n", buf);
          break;
        case VPD_TYPE_READ_ONLY:
          printf("  Read-Only VPD Data:\n");
          _DumpVPDInner(buf, vpdOrigLen, &ckOffset);
          break;
        case VPD_TYPE_READ_WRITE:
          printf("  Read-Write VPD Data:\n");
          _DumpVPDInner(buf, vpdOrigLen, NULL);
          break;
        case VPD_TYPE_END:
          printf("  End of VPD data\n");

          if (ckOffset) {
            uint8_t sum = 0;
            uint8_t *ckRangeEnd = vpdStart + ckOffset + 1;
            for (vpd = vpdStart; vpd != ckRangeEnd; ++vpd)
              sum += *vpd;
            if (sum) {
              printf("  VPD RO Checksum:        MISMATCH (0x%02X)\n", sum);
              errCRC = true;
            } else
              printf("  VPD RO Checksum:        OK\n");
          }

          return errCRC ? 1 : 0;
        default:
          printf("  (Unknown VPD Data)\n");
          break;
        }

        state = STATE_DRIFTING;
        --vpd;
        break;
      }

      *bufp++ = *vpd;
      --vpdLen;
      break;

    default:
      return -1;
    }
  }

  printf("  Warning: malformed VPD data\n");
  return 1;
}

static size_t _extVPDIdx = SIZE_MAX;
static size_t _extDirIdx = SIZE_MAX;
static size_t _extVPDOffset = 0;
static size_t _extDirOffset = 0;
static size_t _extVPDSize = 0;
static size_t _extDirSize = 0;

static int _DumpDirectory(otg_directory_entry *dir, size_t numEntries) {
  char typebuf[16];
  for (size_t i=0; i<numEntries; ++i) {
    const char *typep = typebuf;
    uint32_t type = (ntohl(dir[i].typeSize) & 0xFF000000);
    uint32_t middleBits = (ntohl(dir[i].typeSize) & 0x00C00000) >> 22;
    uint32_t low22 = (ntohl(dir[i].typeSize) & 0x007FFFFF);
    uint32_t loadAddr   = (ntohl(dir[i].loadAddr));
    uint32_t offset     = (ntohl(dir[i].offset));

    switch (type) {
      case OTG_HEADER_TAG_TYPE__APE_CODE:
        typep = "APE code";
        break;
      case OTG_HEADER_TAG_TYPE__EXTENDED_VPD:
        typep = "Extended VPD";
        if (low22) {
          _extVPDIdx = i;
          _extVPDOffset = offset;
          _extVPDSize   = low22*4;
        }
        break;
      case OTG_HEADER_TAG_TYPE__ISCSI_BOOT:
        typep = "iSCSI boot ROM";
        break;
      case OTG_HEADER_TAG_TYPE__ISCSI_CFG:
        typep = "iSCSI configuration";
        break;
      case OTG_HEADER_TAG_TYPE__ISCSI_CFG_PRG:
        typep = "iSCSI configuration program";
        break;
      case OTG_HEADER_TAG_TYPE__ISCSI_CFG_1:
        typep = "iSCSI configuration (1)";
        break;
      case OTG_HEADER_TAG_TYPE__EXT_DIR:
        typep = "Extended directory";
        if (low22) {
          _extDirIdx = i;
          _extDirOffset = offset;
          _extDirSize   = low22*4;
        }
        break;
      case OTG_HEADER_TAG_TYPE__PXE:
        if (low22) {
          typep = "PXE expansion ROM";
          break;
        }
      default:
        snprintf(typebuf, sizeof(typebuf), "type 0x%02X", type>>24);
        break;
    }

    if (type || low22 || middleBits || loadAddr || offset)
      printf("  %2u: [%02X] %-25s [%01X], size=0x%08X, offset=0x%08X, loadAddr=0x%08X\n", i, type>>24, typep, middleBits, low22*4, offset, loadAddr);
/*
    switch (type) {
// Expansion ROM Pointer. Offset of Expansion ROM is in tag.v1, size probably
// in header low 24 bits. v2 probably unused.
OTG_HEADER_TAG_TYPE__PXE           = 0x00<<24,

OTG_HEADER_TAG_TYPE__ASF_INIT      = 0x01<<24,
OTG_HEADER_TAG_TYPE__ASF_CPUA      = 0x02<<24,
OTG_HEADER_TAG_TYPE__ASF_CPUB      = 0x03<<24,
OTG_HEADER_TAG_TYPE__ASF_CFG       = 0x04<<24,
OTG_HEADER_TAG_TYPE__ISCSI_CFG     = 0x05<<24,
OTG_HEADER_TAG_TYPE__ISCSI_CFG_PRG = 0x06<<24,
OTG_HEADER_TAG_TYPE__USER_BLOCK    = 0x07<<24,
OTG_HEADER_TAG_TYPE__BRSF_BLOCK    = 0x08<<24,
OTG_HEADER_TAG_TYPE__ASF_MBOX      = 0x0A<<24,
OTG_HEADER_TAG_TYPE__ISCSI_CFG_1   = 0x0B<<24,
OTG_HEADER_TAG_TYPE__APE_CFG       = 0x0C<<24,

// Pointer to NCSI (that is, APE) executable image.
// v1 is the offset in bytes in flash, header low22 is the size in
// words. v2 unused.
OTG_HEADER_TAG_TYPE__APE_CODE      = 0x0D<<24,

OTG_HEADER_TAG_TYPE__APE_UPDATE    = 0x0E<<24,
OTG_HEADER_TAG_TYPE__EXT_CFG       = 0x0F<<24,
OTG_HEADER_TAG_TYPE__EXT_DIR       = 0x10<<24,
OTG_HEADER_TAG_TYPE__APE_DATA      = 0x11<<24,
OTG_HEADER_TAG_TYPE__APE_WEB_DATA  = 0x12<<24,
OTG_HEADER_TAG_TYPE__APE_WORKAROUND= 0x13<<24,

// Some weird thing handed by stage2 main loop to allow update of some sort
// of VPD data during normal operation. Details unknown.
OTG_HEADER_TAG_TYPE__EXTENDED_VPD          = 0x14<<24,

      case OTG_HEADER_TAG_TYPE__PXE:
      case OTG_HEADER_TAG_TYPE__APE_CODE:

      case OTG_HEADER_TAG_TYPE__APE_CODE:
        printf("  APE code, offset=0x%08X, size=0x%08X (%s)\n",
          v1, low22*4, _Humanize(low22*4, sizeBuf));
        break;
      case OTG_HEADER_TAG_TYPE__EXTENDED_VPD:
        printf("  Extended VPD, offset=0x%08X, size=0x%08X (%s)\n",
          v1, low22*4, _Humanize(low22*4, sizeBuf));
        break;
      case OTG_HEADER_TAG_TYPE__PXE:
        if (low22) {
          printf("  PXE expansion ROM, offset=0x%08X, probable size=0x%08X (%s)\n", v1, low22*4, _Humanize(low22*4, sizeBuf));
          break;
        }
      default:
        printf("  type 0x%02x, offset=0x%06x, v1=0x%08X, v2=0x%08X\n",
          ntohl(hdr->tags[i].header) >> 24,
          ntohl(hdr->tags[i].header) & 0x00FFFFFF,
          ntohl(hdr->tags[i].v1),
          ntohl(hdr->tags[i].v2));
        break;
    }
*/
  }

  return 0;
}

static int _CmdInfo(int pargc, int argc, char **argv) {
  int ec;
  int argidx;
  error_t argerr = argp_parse(&_argpInfo, argc, argv, 0, &argidx, NULL);
  if (argerr || !argv[argidx] || argv[argidx+1]) {
    argp_help(&_argpInfo, stderr, ARGP_HELP_STD_USAGE, argv[0]);
    return 2;
  }

  int fd = open(argv[argidx], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "error: can't open \"%s\"\n", argv[argidx]);
    return 1;
  }

  struct stat st;
  ec = fstat(fd, &st);
  if (ec < 0)
    return 1;

  void *virt = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (!virt)
    return 1;

  void *virtEnd = (uint8_t*)virt + st.st_size;
  otg_header *hdr = (otg_header*)virt;

  if ((void*)(hdr+1) > virtEnd) {
    fprintf(stderr, "error: file too short to have a valid header\n");
    return 1;
  }

  if (ntohl(hdr->magic) != HEADER_MAGIC) {
    fprintf(stderr, "error: not a valid image (bad magic)\n");
    return 1;
  }

  bool errCRC = false;
  bool errS2  = false;
  bool errVPD = false;
  {
    char sizeBuf[HUMAN_SIZE_LEN];
    char name[sizeof(hdr->partNo)+1] = {};
    memcpy(name, hdr->partNo, sizeof(hdr->partNo));

    uint32_t s1Size = ntohl(hdr->s1Size)*4;
    uint32_t s1Offset = ntohl(hdr->s1Offset);

    otg_s2header *s2hdr = (otg_s2header*)((uint8_t*)virt+s1Offset+s1Size);
    if ((void*)(s2hdr + sizeof(otg_s2header)) > virtEnd)
      s2hdr = NULL;

    if (s2hdr && ntohl(s2hdr->magic) != HEADER_MAGIC)
      s2hdr = NULL;

    if (s2hdr && (void*)((uint8_t*)s2hdr + ntohl(s2hdr->s2Size)) > virtEnd)
      s2hdr = NULL;

    printf("========== Image Information ==========\n");
    printf("Total Size:                       0x%08X (%s)\n", st.st_size, _Humanize(st.st_size, sizeBuf));
    printf("Stage 1 Load Addr/Entry:          0x%08X\n", ntohl(hdr->s1Entrypoint));
    printf("Stage 1 Size:                     0x%08X (%s)\n", s1Size, _Humanize(s1Size, sizeBuf));
    printf("Stage 1 Offset:                   0x%08X\n", s1Offset);
    {
      uint32_t expectedCRC = ntohl(hdr->bootHdrCRC);
      uint32_t actualCRC   = SwapEndian32(ComputeCRC((uint8_t*)hdr, 4, 0xFFFFFFFF) ^ 0xFFFFFFFF);
      printf("Boot Header Checksum:             %s\n", (actualCRC == expectedCRC) ? "OK" : "MISMATCH");
      if (actualCRC != expectedCRC) {
        printf("  got 0x%08X, expected 0x%08X\n", actualCRC, expectedCRC);
        errCRC = true;
      }
    }
    {
      uint8_t sum = hdr->dirCRC;
      for (size_t i=0x14; i<0x74; ++i)
        sum += *((uint8_t*)hdr + i);
      printf("Directory Checksum:               %s\n", (!sum) ? "OK" : "MISMATCH");
      if (sum)
        errCRC = true;
    }
    if (ntohs(hdr->mfrLen) != 0x008C)
      printf("WARNING: Unexpected manufacturing data length.\n");
    if (ntohs(hdr->mfr2Len) != 0x008C)
      printf("WARNING: Unexpected manufacturing data 2 length.\n");
    {
      uint32_t expectedCRC = ntohl(hdr->mfrCRC);
      uint32_t actualCRC = SwapEndian32(ComputeCRC(&hdr->mfrFormatRev, 0x008C/4 - 1, 0xFFFFFFFF) ^ 0xFFFFFFFF);
      printf("Manufacturing Data Checksum:      %s\n", (actualCRC == expectedCRC) ? "OK" : "MISMATCH");
      if (actualCRC != expectedCRC) {
        printf("  got 0x%08X, expected 0x%08X\n", actualCRC, expectedCRC);
        errCRC = true;
      }
    }
    {
      uint32_t expectedCRC = ntohl(hdr->mfr2CRC);
      uint32_t actualCRC = SwapEndian32(ComputeCRC(&hdr->mfr2Unk, 0x008C/4 - 1, 0xFFFFFFFF) ^ 0xFFFFFFFF);
      printf("Manufacturing Data 2 Checksum:    %s\n", (actualCRC == expectedCRC) ? "OK" : "MISMATCH");
      if (actualCRC != expectedCRC) {
        printf("  got 0x%08X, expected 0x%08X\n", actualCRC, expectedCRC);
        errCRC = true;
      }
    }
    {
      uint32_t expectedCRC = ntohl(*(uint32_t*)((uint8_t*)hdr + s1Offset + s1Size-4));
      uint32_t actualCRC = SwapEndian32(ComputeCRC((uint8_t*)hdr + s1Offset, (s1Size/4)-1, 0xFFFFFFFF) ^ 0xFFFFFFFF);
      printf("Stage 1 Checksum:                 %s\n", (actualCRC == expectedCRC) ? "OK" : "MISMATCH");
      if (actualCRC != expectedCRC) {
        printf("  got 0x%08X, expected 0x%08X\n", actualCRC, expectedCRC);
        errCRC = true;
      }
    }
    {
      uint32_t virtPtr = ntohl(*(uint32_t*)((uint8_t*)hdr + s1Offset + 8));
      uint32_t offset = virtPtr - ntohl(hdr->s1Entrypoint) + s1Offset;
      printf("Stage 1 Image Version String:     \"%s\"\n", (char*)((uint8_t*)hdr + offset));
    }

    printf("Stage 2 Offset:                   0x%08X\n", s1Offset+s1Size);
    if (s2hdr) {
      printf("Stage 2 Entrypoint:               0x%08X\n", s1Offset+s1Size+8);
      printf("Stage 2 Size:                     0x%08X (%s)\n", ntohl(s2hdr->s2Size), _Humanize(ntohl(s2hdr->s2Size), sizeBuf));

      uint32_t expectedCRC = ntohl(*(uint32_t*)(
          (uint8_t*)s2hdr + 8 + ntohl(s2hdr->s2Size) - 4));
      uint32_t actualCRC = SwapEndian32(ComputeCRC((uint8_t*)s2hdr + 8, (ntohl(s2hdr->s2Size)-4)/4, 0xFFFFFFFF) ^ 0xFFFFFFFF);

      printf("Stage 2 Checksum:                 %s\n", (actualCRC == expectedCRC) ? "OK" : "MISMATCH");
      if (actualCRC != expectedCRC) {
        printf("  got 0x%08X, expected 0x%08X\n", actualCRC, expectedCRC);
        errCRC = true;
      }
    } else {
      printf("Stage 2 invalid (bad magic or file too short)\n");
      errS2 = true;
    }

    printf("Image Name:                       \"%s\"\n", name);
    printf("Directory:\n");
    _DumpDirectory(hdr->dir, ARRAYLEN(hdr->dir));

    printf("PCI Vendor/Device ID:     %04X:%04X\n", ntohs(hdr->pciVendor), ntohs(hdr->pciDevice));
    printf("PCI Subsystem Vendor ID:  %04X\n", ntohs(hdr->pciSubsystemVendor));
    printf("PCI Subsystem ID:\n");
    printf("          Fun0 Fun1 Fun2 Fun3\n");
    printf("  GPHY    %04X %04X %04X %04X\n", ntohs(hdr->pciSubsystemF0GPHY), ntohs(hdr->pciSubsystemF1GPHY), ntohs(hdr->pciSubsystemF2GPHY), ntohs(hdr->pciSubsystemF3GPHY));
    printf("  SERDES  %04X %04X %04X %04X\n", ntohs(hdr->pciSubsystemF0SERDES), ntohs(hdr->pciSubsystemF1SERDES), ntohs(hdr->pciSubsystemF2SERDES), ntohs(hdr->pciSubsystemF3SERDES));

    printf("F0 MAC: %04X%08X\n", ntohl(hdr->mac0[0]), ntohl(hdr->mac0[1]));
    printf("F1 MAC: %04X%08X\n", ntohl(hdr->mac1[0]), ntohl(hdr->mac1[1]));
    printf("F2 MAC: %04X%08X\n", ntohl(hdr->mac2[0]), ntohl(hdr->mac2[1]));
    printf("F3 MAC: %04X%08X\n", ntohl(hdr->mac3[0]), ntohl(hdr->mac3[1]));

    printf("F0 CFG_1E4: 0x%08X\n", ntohl(hdr->func0CfgFeature));
    printf("F0 CFG_2:   0x%08X\n", ntohl(hdr->func0CfgHW));
    printf("F0 CFG_2A8: 0x%08X\n", ntohl(hdr->func0CfgHW2));
    printf("F1 CFG_1E4: 0x%08X\n", ntohl(hdr->func1CfgFeature));
    printf("F1 CFG_2:   0x%08X\n", ntohl(hdr->func1CfgHW));
    printf("F1 CFG_2A8: 0x%08X\n", ntohl(hdr->func1CfgHW2));
    printf("F2 CFG_1E4: 0x%08X\n", ntohl(hdr->func2CfgFeature));
    printf("F2 CFG_2:   0x%08X\n", ntohl(hdr->func2CfgHW));
    printf("F2 CFG_2A8: 0x%08X\n", ntohl(hdr->func2CfgHW2));
    printf("F3 CFG_1E4: 0x%08X\n", ntohl(hdr->func3CfgFeature));
    printf("F3 CFG_2:   0x%08X\n", ntohl(hdr->func3CfgHW));
    printf("F3 CFG_2A8: 0x%08X\n", ntohl(hdr->func3CfgHW2));
    printf("CFG_3:      0x%08X\n", ntohl(hdr->cfgShared));
    printf("CFG_5:      0x%08X\n", ntohl(hdr->cfg5));

    printf("VPD:\n");
    errVPD = errVPD || _DumpVPD((uint8_t*)hdr->vpd, sizeof(hdr->vpd));
  }

  printf("Extended VPD:\n");
  if (_extVPDIdx == SIZE_MAX)
    printf("  Not present\n");
  else {
    uint8_t *extVPD = (uint8_t*)hdr + _extVPDOffset;
    uint8_t *extVPDEnd = extVPD + _extVPDSize;
    if (extVPDEnd > (uint8_t*)virtEnd)
      return 1;
    errVPD = errVPD || _DumpVPD(extVPD, _extVPDSize);
  }

  printf("Extended Directory:\n");
  if (_extDirIdx == SIZE_MAX)
    printf("  Not present\n");
  else {
    uint8_t *extDir = (uint8_t*)hdr + _extDirOffset;
    uint8_t *extDirEnd = extDir + _extDirSize;
    if (extDirEnd > (uint8_t*)virtEnd)
      return 1;
    if ((_extDirSize-4) % sizeof(otg_directory_entry))
      return 1;

    {
      uint32_t expectedCRC = ntohl(*(uint32_t*)(extDirEnd-4));
      uint32_t actualCRC = SwapEndian32(ComputeCRC(extDir, (_extDirSize/4) - 1, 0xFFFFFFFF) ^ 0xFFFFFFFF);
      printf("  Checksum:      %s\n", (actualCRC == expectedCRC) ? "OK" : "MISMATCH");
      if (actualCRC != expectedCRC) {
        printf("    got 0x%08X, expected 0x%08X\n", actualCRC, expectedCRC);
        errCRC = true;
      }
    }

    _DumpDirectory((otg_directory_entry*)extDir, _extDirSize/sizeof(otg_directory_entry));
  }

  if (errCRC || errS2)
    printf("Defects:%s%s%s\n", errCRC ? " crc" : "", errS2 ? " stage2" : "", errVPD ? " vpd" : "");
  else
    printf("Defects: none\n");
  return 0;
}

static const struct argp _argpSet = {
  .doc = "Set a parameter in a firmware image.\v"
    "Parameters:\n"
    "  mac0  ] - MAC addresses.\n"
    "  mac1  ]   Format: 1122aabb1122\n"
    "  mac2  ]\n"
    "  mac3  ]\n"
    "  vpd     - Set VPD data block. Pass as the value a filename to\n"
    "            binary VPD data in the correct format (or /dev/stdin).\n"
    "            The data is copied to the extended VPD region if present;\n"
    "            otherwise it is copied to the standard VPD region.\n"
    "  vpdstd  - Like vpd, but always copies to the standard VPD region.\n"
    "            Can be used to update the standard VPD region when an\n"
    "            extended VPD region is present.\n"
    "  vpdext  - Like vpd, but always copies to the extended VPD region.\n"
    "            Fails if extended VPD region is not present.\n"
    ,
  .args_doc = "<image-filename> <parameter-name> <value>",
};

typedef struct {
  const char *name;
  size_t offset;
  uint8_t type;
} param_t;

enum {
  PARAM_TYPE_MAC = 1,
  PARAM_TYPE_VPD,
  PARAM_TYPE_VPD_STD,
  PARAM_TYPE_VPD_EXT,
};

static const param_t _params[] = {
#define X(Name, FieldName, Type) {(#Name), offsetof(otg_header, FieldName), (PP_CAT(PARAM_TYPE_,Type)),},
  X(mac0, mac0, MAC)
  X(mac1, mac1, MAC)
  X(mac2, mac2, MAC)
  X(mac3, mac3, MAC)
  X(vpd,    vpd, VPD)
  X(vpdstd, vpd, VPD_STD)
  X(vpdext, vpd, VPD_EXT)
#undef X
  {},
};

static int _CmdSet(int pargc, int argc, char **argv) {
  int ec;
  int argidx;
  error_t argerr = argp_parse(&_argpSet, argc, argv, 0, &argidx, NULL);
  if (argc < 4 || argerr || !argv[argidx] || !argv[argidx+1] || !argv[argidx+2] || argv[argidx+3]) {
    argp_help(&_argpSet, stderr, ARGP_HELP_STD_USAGE, argv[0]);
    return 2;
  }

  int fd = open(argv[argidx], O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "error: can't open \"%s\"\n", argv[argidx]);
    return 1;
  }

  struct stat st;
  ec = fstat(fd, &st);
  if (ec < 0)
    return 1;

  void *virt = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (!virt)
    return 1;

  void *virtEnd = (uint8_t*)virt + st.st_size;
  otg_header *hdr = (otg_header*)virt;

  if ((void*)(hdr+1) > virtEnd) {
    fprintf(stderr, "error: file too short to have a valid header\n");
    return 1;
  }

  if (ntohl(hdr->magic) != HEADER_MAGIC) {
    fprintf(stderr, "error: not a valid image (bad magic)\n");
    return 1;
  }

  const char *param = argv[argidx+1];
  const char *value = argv[argidx+2];
  const param_t *pdef;
  for (pdef = _params; pdef->name; ++pdef)
    if (!strcmp(pdef->name, param))
      break;
  if (!pdef->name) {
    fprintf(stderr, "error: unknown parameter name \"%s\"\n", param);
    return 2;
  }

  if (ntohs(hdr->mfrLen) != 0x008C) {
    printf("Unexpected manufacturing data length, cannot continue.\n");
    return 1;
  }
  if (ntohs(hdr->mfr2Len) != 0x008C) {
    printf("Unexpected manufacturing data 2 length, cannot continue.\n");
    return 1;
  }

  bool goodMfrCRC1, goodMfrCRC2;
  {
    uint32_t expectedCRC = ntohl(hdr->mfrCRC);
    uint32_t actualCRC = SwapEndian32(ComputeCRC(&hdr->mfrFormatRev, 0x008C/4 - 1, 0xFFFFFFFF) ^ 0xFFFFFFFF);
    goodMfrCRC1 = (actualCRC == expectedCRC);
  }
  {
    uint32_t expectedCRC = ntohl(hdr->mfr2CRC);
    uint32_t actualCRC = SwapEndian32(ComputeCRC(&hdr->mfr2Unk, 0x008C/4 - 1, 0xFFFFFFFF) ^ 0xFFFFFFFF);
    goodMfrCRC2 = (actualCRC == expectedCRC);
  }

  bool changeTouchesMfr  = (pdef->offset >= 0x074 && pdef->offset < 0x0FC);
  bool changeTouchesMfr2 = (pdef->offset >= 0x200 && pdef->offset < 0x288);

  unsigned mac32[6];
  uint8_t mac[6];
  uint32_t type = pdef->type;
  switch (type) {
    case PARAM_TYPE_MAC:
      if (sscanf(value, "%02x%02x%02x%02x%02x%02x", &mac32[0], &mac32[1], &mac32[2], &mac32[3], &mac32[4], &mac32[5]) < 6) {
        fprintf(stderr, "error: malformed MAC address\n");
        return 2;
      }

      mac[0] = (uint8_t)mac32[0];
      mac[1] = (uint8_t)mac32[1];
      mac[2] = (uint8_t)mac32[2];
      mac[3] = (uint8_t)mac32[3];
      mac[4] = (uint8_t)mac32[4];
      mac[5] = (uint8_t)mac32[5];
      ssize_t wr = pwrite(fd, mac, 6, pdef->offset+2);
      if (wr < 6) {
        fprintf(stderr, "error: failed to write value\n");
        return 1;
      }

      break;

    case PARAM_TYPE_VPD:
    case PARAM_TYPE_VPD_STD:
    case PARAM_TYPE_VPD_EXT: {
      int vpdIdx = -1;
      if (type != PARAM_TYPE_VPD_STD)
        for (size_t i=0; i<ARRAYLEN(hdr->dir); ++i) {
          if ((ntohl(hdr->dir[i].typeSize) & 0xFF000000) == OTG_HEADER_TAG_TYPE__EXTENDED_VPD && ntohl(hdr->dir[i].offset)) {
            vpdIdx = i;
            break;
          }
        }
      if (type == PARAM_TYPE_VPD)
        type = (vpdIdx >= 0) ? PARAM_TYPE_VPD_EXT : PARAM_TYPE_VPD_STD;

      if (type == PARAM_TYPE_VPD_EXT && vpdIdx < 0) {
        fprintf(stderr, "error: extended VPD area not present\n");
        return 1;
      }

      uint32_t vpdStart;
      uint32_t vpdLen;
      if (type == PARAM_TYPE_VPD_STD) {
        vpdStart = 0x100;
        vpdLen   = sizeof(hdr->vpd);
      } else {
        vpdStart = ntohl(hdr->dir[vpdIdx].offset);
        vpdLen   = (ntohl(hdr->dir[vpdIdx].typeSize) & 0x3FFFFF)*4;
      }

      void *vpdBuf = calloc(1, vpdLen+1);
      assert(vpdBuf);

      FILE *fi = fopen(value, "rb");
      if (!fi) {
        fprintf(stderr, "error: could not open file: %s\n", value);
        return 1;
      }

      ssize_t rd = fread(vpdBuf, 1, vpdLen+1, fi);
      if (rd < 0) {
        fprintf(stderr, "error reading file\n");
        return 1;
      }

      if (rd > vpdLen) {
        fprintf(stderr, "error: VPD data is too large to fit\n");
        return 1;
      }

      ssize_t wr = pwrite(fd, vpdBuf, vpdLen, vpdStart);
      if (wr < vpdLen) {
        fprintf(stderr, "error: failed to write value\n");
        return 1;
      }

    } break;

    default:
      abort();
  }

  if ((changeTouchesMfr && !goodMfrCRC1) || (changeTouchesMfr2 && !goodMfrCRC2)) {
    fprintf(stderr,
      "WARNING: The CRCs for the manufacturing data block containing the specified field\n"
      "  is not valid in this image. This CRC is not checked during device boot, so in\n"
      "  practice this is harmless and setting the value will still work.\n"
      "  \n"
      "  However, it is possible that some of manufacturing data has been corrupted. Another\n"
      "  possibility is that the image was simply mis-manufactured and the factory isn't setting\n"
      "  the manufacturing data block CRCs correctly (because they aren't required for correct\n"
      "  operation, it's easily conceivable a factory would neglect to do this, and there seem\n"
      "  to be some instances of this actually happening in the field.)\n"
      "  \n"
      "  Because otgimg can't tell which is the case (factory screwup or actual data corruption),\n"
      "  otgimg won't write a correct manufacturing data block CRC like it normally would, because\n"
      "  by doing so it might give a correct CRC to data which has in fact been corrupted.\n"
      "  Instead, it will leave the old CRCs in place and not update them as it normally would.\n"
      "  Since this CRC is not checked at runtime, this should not adversely affect device\n"
      "  operation.\n" 
      "  \n"
      "  Values have been set as asked and CRC field has *not* been updated.\n"
      );
  } else {
    if (changeTouchesMfr) {
      uint32_t crc = htole32(ComputeCRC(&hdr->mfrFormatRev, 0x008C/4 - 1, 0xFFFFFFFF) ^ 0xFFFFFFFF);
      ssize_t wr = pwrite(fd, &crc, sizeof(crc), 0x0FC);
      if (wr < sizeof(crc))
        return 1;
    }
    if (changeTouchesMfr2) {
      uint32_t crc = htole32(ComputeCRC(&hdr->mfr2Unk, 0x008C/4 - 1, 0xFFFFFFFF) ^ 0xFFFFFFFF);
      ssize_t wr = pwrite(fd, &crc, sizeof(crc), 0x288);
      if (wr < sizeof(crc))
        return 1;
    }
  }

  return 0;
}

static const struct argp _argp = {
  .args_doc = "<command> [command-args...]",
  .doc = "otg firmware image servicing tool.\vCommands:\n"
    "  info     show information about a firmware image\n"
    "  set      set a parameter in a firmware image\n"
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
    .name = "set",
    .func = _CmdSet,
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
