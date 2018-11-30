#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sched.h>
#include <signal.h>
#include "otg.h"
#include "otg_common.c"

#define ARRAYLEN(X) (sizeof(X)/sizeof((X)[0]))

/* MMIO Access
 * -----------
 */
typedef struct mmio mmio_t;

struct mmio {
  void *virt;
  uintptr_t phys;
  size_t len;
};

static inline void *PtrAdd(void *x, uintptr_t y) {
  return (((uint8_t*)x) + y);
}

static int _memFD = -1;

static int _GetMemFD(void) {
  if (_memFD < 0) {
    _memFD = open("/dev/mem", O_RDWR|O_SYNC);
    if (_memFD < 0) {
      fprintf(stderr, "error: couldn't open /dev/mem - are you running as root?\n");
      return -1;
    }
  }

  return _memFD;
}

int MMIOOpen(uintptr_t phys, size_t len, mmio_t **mmio) {
  mmio_t *m = NULL;

  m = calloc(1, sizeof(mmio_t));
  if (!m)
    goto error;

  int fd = _GetMemFD();
  if (fd < 0)
    goto error;

  m->phys = phys;
  m->len = len;
  m->virt = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, phys);
  if (!m->virt) {
    fprintf(stderr, "error: couldn't mmap %p (length 0x%08" PRIuPTR ")\n", (void*)phys, len);
    goto error;
  }

  *mmio = m;
  return 0;

error:
  if (m && m->virt)
    munmap(m->virt, m->len);
  free(m);
  return -1;
}

void MMIOClose(mmio_t *mmio) {
  if (!mmio)
    return;
  if (mmio->virt)
    munmap(mmio->virt, mmio->len);
  free(mmio);
}

static inline uint32_t MMIOGet32(mmio_t *mmio, uintptr_t offset) {
  return *(volatile uint32_t*)PtrAdd(mmio->virt, offset);
}

static inline void MMIOSet32(mmio_t *mmio, uintptr_t offset, uint32_t value) {
  *(volatile uint32_t*)PtrAdd(mmio->virt, offset) = value;
}


/* Device Lookup
 * =============
 */
typedef struct {
  uint64_t physStart, physEnd; // inclusive
  uint64_t flags;
} device_resource_t;

typedef struct {
  uint32_t busAddr; // [domain: 16 bits][bus: 8 bits][device: 5 bits][function: 3 bits]
  uint16_t vendorID, deviceID;
  struct {
    uint64_t phys, len;
  } bars[6];

  size_t configLen;
  union {
    uint8_t configBuf[4096];
    struct __attribute__((packed)) {
      uint16_t vendorID, deviceID;
      uint16_t command, status;
      uint32_t revisionID : 8;
      uint32_t classCode  :24;
      uint8_t  cacheLineSize, latTimer, headerType, bist;
      uint32_t bar[6];
      uint32_t cardbus;
      uint16_t subsystemVendorID, subsystemID;
      uint32_t expansionROMBaseAddr;
      uint8_t  capOffset;
      uint8_t  _reserved[3];
      uint32_t _reserved2;
      uint8_t  interruptLine, interruptPin, minGnt, maxLat;
    } config;
  };

  device_resource_t resources[10];
  int type;
} device_info_t;

/*static inline uintptr_t BARToPhys(device_info_t *info, uint8_t barNo) {
  uintptr_t x;
  info->config[barNo]
  return (((uint64_t)info->config[barNo+1]) << 32) | info->config[barNo];
}*/

static inline uint32_t GetPCIDomainNo(uint32_t busAddr) {
  return busAddr >> 16;
}
static inline uint32_t GetPCIBusNo(uint32_t busAddr) {
  return (busAddr >> 8) & 0xFF;
}
static inline uint32_t GetPCIDeviceNo(uint32_t busAddr) {
  return (busAddr >> 3) & 0x1F;
}
static inline uint32_t GetPCIFuncNo(uint32_t busAddr) {
  return busAddr & 0x07;
}

static inline uint32_t MakePCIBusAddr(uint32_t domain, uint32_t bus, uint32_t device, uint32_t func) {
  if (domain > UINT16_MAX || bus > UINT8_MAX || device > 31 || func > 7)
    return UINT32_MAX;

  return (domain<<16)|(bus<<8)|(device<<3)|func;
}

static const size_t PCI_BUS_STRING_LEN = 13;

static void PCIBusAddrToString(uint32_t busAddr, char *buf, size_t buf_len) {
  snprintf(buf, buf_len, "%04x:%02x:%02x.%01x",
    GetPCIDomainNo(busAddr),
    GetPCIBusNo(busAddr),
    GetPCIDeviceNo(busAddr),
    GetPCIFuncNo(busAddr)
  );
}

/* DeviceResolveByString
 * ---------------------
 * Takes a device specification string and fills the provided device_info_t
 * structure with information about the device.
 *
 * The string can be in any of the following formats:
 *
 *   "BB:DD.F"        - PCI bus address.
 *   "dddd:BB:DD.F"   - Extended PCI bus address.
 *
 * Only the busAddr field is filled. Use DeviceLoadInfo to populate the other
 * fields.
 */
int DeviceResolveByString(const char *str, device_info_t *info) {
  int ec;
  uint32_t domain, bus, device, func;

  ec = sscanf(str, "%04x:%02x:%02x.%01x", &domain, &bus, &device, &func);
  if (ec < 4) {
    domain = 0;
    ec = sscanf(str, "%02x:%02x.%01x", &bus, &device, &func);
    if (ec < 3) {
      fprintf(stderr, "malformed device string: \"%s\" - should be PCI bus address ([dddd:]BB:DD.F)\n", str);
      return -1;
    }
  }

  info->busAddr = MakePCIBusAddr(domain, bus, device, func);
  if (info->busAddr == UINT32_MAX) {
    fprintf(stderr, "malformed device string: \"%s\"", str);
    return -1;
  }

  return 0;
}

int DeviceLoadInfo(device_info_t *info) {
  int fd = -1;
  char busAddrStr[PCI_BUS_STRING_LEN];
  PCIBusAddrToString(info->busAddr, busAddrStr, ARRAYLEN(busAddrStr));

  char devicePath[64+PCI_BUS_STRING_LEN];
  snprintf(devicePath, sizeof(devicePath), "/sys/bus/pci/devices/%s/config", busAddrStr);

  memset(info->configBuf, 0, sizeof(info->configBuf));
  fd = open(devicePath, O_RDONLY|O_SYNC);
  if (fd < 0)
    goto error;

  ssize_t rd = read(fd, info->configBuf, sizeof(info->configBuf));
  if (rd < 256)
    goto error;

  info->configLen = rd;

  close(fd);
  fd = -1;

  // load resources
  {
    snprintf(devicePath, sizeof(devicePath), "/sys/bus/pci/devices/%s/resource", busAddrStr);
    fd = open(devicePath, O_RDONLY|O_SYNC);
    if (fd < 0)
      goto error;

    char buf[1024] = {};
    rd = read(fd, buf, sizeof(buf)-1);
    if (rd < 0)
      goto error;

    buf[sizeof(buf)-1] = 0;
    const char *bufp = buf;
    for (size_t i=0; i<ARRAYLEN(info->resources); ++i) {
      if (sscanf(bufp, "0x%" SCNx64 " 0x%" SCNx64 " 0x%" SCNx64 "\n", &info->resources[i].physStart, &info->resources[i].physEnd, &info->resources[i].flags) < 3)
        info->resources[i].flags = 0;
      bufp = strchr(bufp, '\n')+1;
    }
  }

  return 0;

error:
  if (fd >= 0)
    close(fd);
  fprintf(stderr, "couldn't load device configuration - are you root?\n");
  return -1;
}

static void DevicePrintInfo(device_info_t *info) {
  char busAddrStr[PCI_BUS_STRING_LEN];
  PCIBusAddrToString(info->busAddr, busAddrStr, ARRAYLEN(busAddrStr));

  printf("%s %04x:%04x (%04x:%04x) rev%02x class%06x rom%08x\n", busAddrStr, info->config.vendorID, info->config.deviceID, info->config.subsystemVendorID, info->config.subsystemID, info->config.revisionID, info->config.classCode, info->config.expansionROMBaseAddr);

#if 0
  for (size_t i=0; i<ARRAYLEN(info->config.bar); ++i)
    printf("bar%2" PRIuPTR ")  %08x  %s  %u\n", i, info->config.bar[i], (info->config.bar[i] & 1) ? "(IO) " : "(MEM)",
      (info->config.bar[i] & 1) ? 0 : ((info->config.bar[i] >> 1) & 0x03));
#endif

  for (size_t i=0; i<ARRAYLEN(info->resources); ++i) {
    uint64_t flags = info->resources[i].flags;
    if (!(flags & 0x1F00)) {
      printf("res%2" PRIuPTR ")  --\n", i);
      continue;
    }

    char flagstr[64];
    char *flagptr = flagstr;
    switch (flags & 0x1F00) {
      case 0x0100:
        *flagptr++ = 'I';
        *flagptr++ = 'O';
        *flagptr++ = ' ';
        *flagptr++ = ' ';
        break;
      case 0x0200:
        *flagptr++ = 'M';
        *flagptr++ = 'E';
        *flagptr++ = 'M';
        *flagptr++ = ' ';
      default:
        break;
    }
    *flagptr++ = (flags & 0x00002000) ? 'P' : ' '; // prefetch
    *flagptr++ = (flags & 0x00004000) ? 'R' : ' '; // read only 
    *flagptr++ = (flags & 0x00008000) ? 'C' : ' '; // cacheable
    *flagptr++ = (flags & 0x00040000) ? 'A' : ' '; // size indicates alignment
    *flagptr++ = (flags & 0x00080000) ? 'a' : ' '; // start field is alignment
    *flagptr++ = (flags & 0x00100000) ? 'L' : ' '; // 64-bit
    *flagptr++ = (flags & 0x08000000) ? 'E' : ' '; // exclusive
    *flagptr++ = (flags & 0x10000000) ? 'D' : ' '; // disabled
    *flagptr++ = 0;
    printf("res%2" PRIuPTR ")  %016" PRIx64 " %016" PRIx64 " %016" PRIx64 " %s\n", i, info->resources[i].physStart, info->resources[i].physEnd, info->resources[i].flags, flagstr);
  }
}

typedef struct {
  uint16_t vendorID;
  uint16_t deviceID;
  int type;
} supported_device_t;

enum {
  DEVTYPE_UNKNOWN = -1,
  DEVTYPE_BCM5719 =  1,
};

static const supported_device_t _supportTable[] = {
  {0x14E4, 0x1657, DEVTYPE_BCM5719},
  {0xFFFF, 0xFFFF, DEVTYPE_BCM5719},
  {},
};

static int DeviceDetermineSupport(const device_info_t *info, const supported_device_t *supportTable) {
  uint16_t vendorID = info->config.vendorID;
  uint16_t deviceID = info->config.deviceID;

  for (;supportTable->vendorID;++supportTable)
    if (supportTable->vendorID == vendorID && supportTable->deviceID == deviceID)
      return supportTable->type;

  return -1;
}

#define MMIO_GENCOM_BASE 0x01000B50

static void *g_bar12;
static void *g_bar34;

static inline void *GetBAR12Base(void) {
  return g_bar12;
}
static inline void *GetBAR34Base(void) {
  return g_bar34;
}

device_info_t g_devInfo;
mmio_t *g_mmio12 = NULL;
mmio_t *g_mmio34 = NULL;

static void PrintCommand(int pargc, int argc, char **argv) {
  argv -= pargc;
  while (pargc--)
    fprintf(stderr, "%s ", *argv++);
  fprintf(stderr, "%s ", argv[0]);
}

static void _IssueWarnings(void) {
  static bool _done = false;
  if (_done)
    return;

  bool error = false;
  uint32_t r = GetReg(REG_MISCELLANEOUS_HOST_CONTROL);
  if (r & REG_MISCELLANEOUS_HOST_CONTROL__ENABLE_ENDIAN_WORD_SWAP) {
    fprintf(stderr, "WARNING: device has endian wordswapping enabled\n");
    error = true;
  }

  if (r & REG_MISCELLANEOUS_HOST_CONTROL__ENABLE_ENDIAN_BYTE_SWAP) {
    fprintf(stderr, "WARNING: device has endian byteswapping enabled\n");
    error = true;
  }

  if (error) {
    fprintf(stderr,
      "Access to device memory will be jumbled and otgdbg functions will not\n"
      "work correctly.\n"
      "\n"
      "Not reconfiguring device to fix the above warnings in case the device\n"
      "is in use by a driver which expects the above settings.\n"
      "Run 'set r/0x68&=~0x0C' to set the correct settings.\n"
      );
  }

  _done = true;
}

static int _CmdInfo(int pargc, int argc, char **argv) {
  DevicePrintInfo(&g_devInfo);
  return 0;
}

static int _UsageGet(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: ");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "<address>[+num-words] [<address>[+num-words]...]\n");
  fprintf(stderr,
    "  Retrieves a 32-bit word from device memory at the specified\n"
    "  <address>. The <address> is specified based on the RX CPU's\n"
    "  view of memory (so GENCOM starts at 0x0000_0B50, etc.)\n"
    "  Hex notation is supported. An optional suffix, [+num-words]\n"
    "  may be specified to output multiple consecutive words.\n"
    "  By default only the word specified is output.\n"
    "  Multiple arguments may be specified, which are each processed\n"
    "  individually in sequence.\n"
    );
  if (!strcmp(argv[0], "dump"))
    fprintf(stderr,
    "  The successive words are output in binary, as little-endian values.\n");
  else
    fprintf(stderr,
    "  Each word is output as a hexadecimal value.\n");
  fprintf(stderr,
    "\n"
    "  <address> also supports some optional prefixes before the numeric part: \n"
    "    g/<address>    Retrieve GENCOM value (so g/0 is equivalent to 0x0000_0B50).\n"
    "    r/<address>    Retrieve device register value (so r/0+4 is equivalent to 0xC000_0000+4).\n"
    "    a/<address>    Retrieve APE register value (so a/0 is equivalent to 0xC001_0000).\n"
    "\n"
    "  You also prefix the prefix/address with '+' to force use of the RX CPU IP based read method.\n"
    "  Prefix '.' to force use of the RX CPU forced load method.\n"
    "  Prefix 'w' to force use of the window method.\n"
    "  Prefix 's' to access the APE event scratchpad.\n"
    "  Prefix 'o' to access APE OTP.\n"
    "  Prefix 'i' for experimental APE thingy.\n"
    "  Prefix 'h' to use APE shell. APE shell (apedbg) must be running.\n"
    "  For binary output, use the \"dump\" command instead of \"get\".\n"
    );
  return -2;
}

enum {
  ACCESS_MODE_DEFAULT,
  ACCESS_MODE_IP,
  ACCESS_MODE_FORCED_LOAD,
  ACCESS_MODE_WINDOW,
  ACCESS_MODE_APE_EVENT_SCRATCHPAD,
  ACCESS_MODE_APE_OTP,
  ACCESS_MODE_APE_CODE,
  ACCESS_MODE_APE_SHELL,
};

static int _ResolveAddress(const char *addr, uint32_t *ad, uint32_t *numWords, const char **tailp, int *accessMode) {
  if (!addr)
    return -1;

  if (accessMode) {
    if (addr[0] == '+') {
      *accessMode = ACCESS_MODE_IP;
      ++addr;
    } else if (addr[0] == 'w') {
      *accessMode = ACCESS_MODE_WINDOW;
      ++addr;
    } else if (addr[0] == '.') {
      *accessMode = ACCESS_MODE_FORCED_LOAD;
      ++addr;
    } else if (addr[0] == 's') {
      *accessMode = ACCESS_MODE_APE_EVENT_SCRATCHPAD;
      ++addr;
    } else if (addr[0] == 'o') {
      *accessMode = ACCESS_MODE_APE_OTP;
      ++addr;
    } else if (addr[0] == 'i') {
      *accessMode = ACCESS_MODE_APE_CODE;
      ++addr;
    } else if (addr[0] == 'h') {
      *accessMode = ACCESS_MODE_APE_SHELL;
      ++addr;
    } else
      *accessMode = ACCESS_MODE_DEFAULT;
  }

  if (!*addr)
    return -1;

  char *tail = NULL;
  uint32_t ad_ = 0;
  const char *addr_ = addr+2;
  if (addr[0] == 'g' && addr[1] == '/')
    ad_ = GENCOM_BASE;
  else if (addr[0] == 'r' && addr[1] == '/')
    ad_ = REGMEM_BASE;
  else if (addr[0] == 'a' && addr[1] == '/')
    ad_ = REGMEM_BASE+APE_OFFSET;
  else
    addr_ = addr;

  addr = addr_;

  ad_ += strtoul(addr, &tail, 0);
  if (!tail || tail == addr || (*tail && !tailp && (!numWords || *tail != '+')))
    return -1;
  if (numWords) {
    if (tail && *tail == '+' && numWords) {
      ++tail;
      const char *nw = tail;
      *numWords = strtoul(nw, &tail, 0);
      if (!tail || tail == nw || (*tail && !tailp))
        return -1;
    } else
      *numWords = 1;
  }

  *ad = ad_;
  if (tailp)
    *tailp = tail;
  return 0;
}

static int _CmdGetEx(int pargc, int argc, char **argv, bool dump) {
  if (argc < 2)
    return _UsageGet(pargc, argc, argv);

  int ec;
  char **curArgv = argv+1;
  for (; *curArgv; ++curArgv) {
    uint32_t ad;
    uint32_t numWords;
    int accessMode;
    ec = _ResolveAddress(*curArgv, &ad, &numWords, NULL, &accessMode);
    if (ec < 0)
      return _UsageGet(pargc, argc, argv);

    uint32_t endAt = ad + numWords*4;
    for (; ad < endAt; ad += 4) {
      uint32_t v;
      if (accessMode == ACCESS_MODE_IP)
        v = GetRXWordViaIP(ad);
      else if (accessMode == ACCESS_MODE_FORCED_LOAD)
        v = GetRXWordViaForcedLoad(ad);
      else if (accessMode == ACCESS_MODE_WINDOW)
        v = GetRXWordViaWindow(ad);
      else if (accessMode == ACCESS_MODE_APE_EVENT_SCRATCHPAD)
        v = GetAPEEventScratchpadWord(ad);
      else if (accessMode == ACCESS_MODE_APE_OTP)
        v = GetOTP(ad);
      else if (accessMode == ACCESS_MODE_APE_CODE)
        v = GetAPEWord(ad);
      else if (accessMode == ACCESS_MODE_APE_SHELL)
        v = GetAPEMemShell(ad);
      else
        v = GetRXWord(ad);

      if (dump) {
        ssize_t wr = fwrite(&v, sizeof(v), 1, stdout);
        if (wr < 1)
          return -1;
      } else
        printf("[0x%04X_%04X] = 0x%04X_%04X\n", ad>>16, ad&0xFFFF, v>>16, v&0xFFFF);
    }
  }

  return 0;
}

static int _CmdGet(int pargc, int argc, char **argv) {
  return _CmdGetEx(pargc, argc, argv, false);
}

static int _CmdDump(int pargc, int argc, char **argv) {
  return _CmdGetEx(pargc, argc, argv, true);
}

static int _UsageSet(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: ");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "<address><op><value> [<address><op><value>...]\n");
  fprintf(stderr,
    "  Sets a 32-bit word in device memory or registers at the specified\n"
    "  <address>. The <address> is specified based on the RX CPU's view\n"
    "  of memory (so GENCOM starts at 0x0000_0B50, etc.)\n"
    "  <value> is an unsigned integer.\n"
    "  Hex notation is supported.\n"
    "  \n"
    "  <op> is one of the following:\n"
    "    =    Simple assignment of given value\n"
    "   |=    OR current value of word with given value\n"
    "   &=    AND current value with given value\n"
    "   &=~   AND current value with bitwise NOT of given value (mask bits off)\n"
    "\n"
    "  <address> supports the same optional prefixes that the \"get\" command does\n"
    "  (g/, r/, etc.)\n"
    );
  return -2;
}

static int _CmdSet(int pargc, int argc, char **argv) {
  if (argc < 2)
    return _UsageSet(pargc, argc, argv);

  enum {
    OPC_SET,
    OPC_OR,
    OPC_AND,
    OPC_ANDNOT,
  };

  int ec;
  char **curArgv = argv+1;
  uint32_t ad;
  const char *tail = NULL;
  int accessMode;
  for (; *curArgv; ++curArgv) {
    ec = _ResolveAddress(*curArgv, &ad, NULL, &tail, &accessMode);
    if (ec < 0) {
      fprintf(stderr, "error: bad address input\n");
      return _UsageSet(pargc, argc, argv);
    }

    uint8_t opc;
    if (tail[0] == '=') {
      opc = OPC_SET;
      ++tail;
    } else if (tail[0] == '|' && tail[1] == '=') {
      opc = OPC_OR;
      tail += 2;
    } else if (tail[0] == '&' && tail[1] == '=' && tail[2] == '~') {
      opc = OPC_ANDNOT;
      tail += 3;
    } else if (tail[0] == '&' && tail[1] == '=') {
      opc = OPC_AND;
      tail += 2;
    } else
      return _UsageSet(pargc, argc, argv);

    const char *vbuf = tail;
    uint32_t v = strtoul(vbuf, (char**)&tail, 0);
    if (!tail || tail == vbuf || *tail)
      return _UsageSet(pargc, argc, argv);


    uint32_t (*getFunc)(uint32_t addr);
    void (*setFunc)(uint32_t addr, uint32_t value);

    if (accessMode == ACCESS_MODE_IP) {
      fprintf(stderr, "IP mode cannot be used for stores\n");
      return 1;
    } else if (accessMode == ACCESS_MODE_FORCED_LOAD) {
      getFunc = GetRXWordViaForcedLoad;
      setFunc = SetRXWordViaForcedStore;
    } else if (accessMode == ACCESS_MODE_WINDOW) {
      getFunc = GetRXWordViaWindow;
      setFunc = SetRXWordViaWindow;
    } else if (accessMode == ACCESS_MODE_APE_EVENT_SCRATCHPAD) {
      getFunc = GetAPEEventScratchpadWord;
      setFunc = SetAPEEventScratchpadWord;
    } else if (accessMode == ACCESS_MODE_APE_OTP) {
      abort(); // TODO
    } else if (accessMode == ACCESS_MODE_APE_CODE) {
      getFunc = GetAPEWord;
      setFunc = SetAPEWord;
    } else if (accessMode == ACCESS_MODE_APE_SHELL) {
      getFunc = GetAPEMemShell;
      setFunc = SetAPEMemShell;
    } else {
      getFunc = GetRXWord;
      setFunc = SetRXWord;
    }

    switch (opc) {
      case OPC_SET:
        setFunc(ad, v);
        break;
      case OPC_OR:
        setFunc(ad, getFunc(ad)|v);
        break;
      case OPC_ANDNOT:
        setFunc(ad, getFunc(ad) & ~v);
        break;
      case OPC_AND:
        setFunc(ad, getFunc(ad) & v);
        break;
    }
  }

  return 0;
}

static int _CmdHalt(int pargc, int argc, char **argv) {
  uint32_t mode = GetReg(REG_RX_RISC_MODE);

  if (mode & REG_RX_RISC_MODE__ENABLE_WATCHDOG) {
    printf(
      "Warning: disabling watchdog, as it unhalts the CPU.\n"
      "This change will not be automatically reverted when resuming.\n");

    mode &= ~REG_RX_RISC_MODE__ENABLE_WATCHDOG;
  }

  mode |= REG_RX_RISC_MODE__HALT;

  SetReg(REG_RX_RISC_MODE, mode);
  return 0;
}

static int _CmdResume(int pargc, int argc, char **argv) {
  uint32_t clearables = GetReg(REG_RX_RISC_STATUS) & REG_RX_RISC_STATUS__CLEARABLE_MASK;
  if (clearables) {
    if (argc > 1 && !strcmp(argv[1], "-f"))
      SetReg(REG_RX_RISC_STATUS, clearables);
    else
      printf("There exist exception conditions which have caused the RX CPU to halt.\n"
        "Add the -f option to clear these conditions.\n"
        );
  }

  MaskReg(REG_RX_RISC_MODE, REG_RX_RISC_MODE__HALT);
  return 0;
}

static int _CmdStep(int pargc, int argc, char **argv) {
  OrReg(REG_RX_RISC_MODE, REG_RX_RISC_MODE__SINGLE_STEP);
  return 0;
}

static int _CmdCPUInfo(int pargc, int argc, char **argv) {
  char flags[15] = {};

  uint32_t oldMode = GetReg(REG_RX_RISC_MODE);
  uint32_t oldStatus = GetReg(REG_RX_RISC_STATUS);
  if (!(oldMode & REG_RX_RISC_MODE__HALT)) {
    printf("Warning: CPU not halted.\n");
    SetReg(REG_RX_RISC_MODE, oldMode|REG_RX_RISC_MODE__HALT);
  }

  uint32_t ip = GetReg(REG_RX_RISC_PROGRAM_COUNTER);
  uint32_t iw = GetReg(REG_RX_RISC_CUR_INSTRUCTION);
  uint32_t at = GetReg(REG_RX_RISC_REG_AT);
  uint32_t v0 = GetReg(REG_RX_RISC_REG_V0);
  uint32_t v1 = GetReg(REG_RX_RISC_REG_V1);
  uint32_t a0 = GetReg(REG_RX_RISC_REG_A0);
  uint32_t a1 = GetReg(REG_RX_RISC_REG_A1);
  uint32_t a2 = GetReg(REG_RX_RISC_REG_A2);
  uint32_t a3 = GetReg(REG_RX_RISC_REG_A3);
  uint32_t t0 = GetReg(REG_RX_RISC_REG_T0);
  uint32_t t1 = GetReg(REG_RX_RISC_REG_T1);
  uint32_t t2 = GetReg(REG_RX_RISC_REG_T2);
  uint32_t t3 = GetReg(REG_RX_RISC_REG_T3);
  uint32_t t4 = GetReg(REG_RX_RISC_REG_T4);
  uint32_t t5 = GetReg(REG_RX_RISC_REG_T5);
  uint32_t t6 = GetReg(REG_RX_RISC_REG_T6);
  uint32_t t7 = GetReg(REG_RX_RISC_REG_T7);
  uint32_t s0 = GetReg(REG_RX_RISC_REG_S0);
  uint32_t s1 = GetReg(REG_RX_RISC_REG_S1);
  uint32_t s2 = GetReg(REG_RX_RISC_REG_S2);
  uint32_t s3 = GetReg(REG_RX_RISC_REG_S3);
  uint32_t s4 = GetReg(REG_RX_RISC_REG_S4);
  uint32_t s5 = GetReg(REG_RX_RISC_REG_S5);
  uint32_t s6 = GetReg(REG_RX_RISC_REG_S6);
  uint32_t s7 = GetReg(REG_RX_RISC_REG_S7);
  uint32_t t8 = GetReg(REG_RX_RISC_REG_T8);
  uint32_t t9 = GetReg(REG_RX_RISC_REG_T9);
  uint32_t k0 = GetReg(REG_RX_RISC_REG_K0);
  uint32_t k1 = GetReg(REG_RX_RISC_REG_K1);
  uint32_t gp = GetReg(REG_RX_RISC_REG_GP);
  uint32_t sp = GetReg(REG_RX_RISC_REG_SP);
  uint32_t fp = GetReg(REG_RX_RISC_REG_FP);
  uint32_t ra = GetReg(REG_RX_RISC_REG_RA);

  printf("============= RX =============\n");
  printf("IP=0x%04X_%04X  IW=0x%04X_%04X\n", ip>>16, ip&0xFFFF, iw>>16, iw&0xFFFF);
  printf("%-14s  s0=0x%04X_%04X\n", flags,             s0>>16, s0&0xFFFF);
  printf("at=0x%04X_%04X  s1=0x%04X_%04X\n", at>>16, at&0xFFFF, s1>>16, s1&0xFFFF);
  printf("v0=0x%04X_%04X  s2=0x%04X_%04X\n", v0>>16, v0&0xFFFF, s2>>16, s2&0xFFFF);
  printf("v1=0x%04X_%04X  s3=0x%04X_%04X\n", v1>>16, v1&0xFFFF, s3>>16, s3&0xFFFF);
  printf("a0=0x%04X_%04X  s4=0x%04X_%04X\n", a0>>16, a0&0xFFFF, s4>>16, s4&0xFFFF);
  printf("a1=0x%04X_%04X  s5=0x%04X_%04X\n", a1>>16, a1&0xFFFF, s5>>16, s5&0xFFFF);
  printf("a2=0x%04X_%04X  s6=0x%04X_%04X\n", a2>>16, a2&0xFFFF, s6>>16, s6&0xFFFF);
  printf("a3=0x%04X_%04X  s7=0x%04X_%04X\n", a3>>16, a3&0xFFFF, s7>>16, s7&0xFFFF);
  printf("t8=0x%04X_%04X  t0=0x%04X_%04X\n", t8>>16, t8&0xFFFF, t0>>16, t0&0xFFFF);
  printf("t9=0x%04X_%04X  t1=0x%04X_%04X\n", t9>>16, t9&0xFFFF, t1>>16, t1&0xFFFF);
  printf("k0=0x%04X_%04X  t2=0x%04X_%04X\n", k0>>16, k0&0xFFFF, t2>>16, t2&0xFFFF);
  printf("k1=0x%04X_%04X  t3=0x%04X_%04X\n", k1>>16, k1&0xFFFF, t3>>16, t3&0xFFFF);
  printf("gp=0x%04X_%04X  t4=0x%04X_%04X\n", gp>>16, gp&0xFFFF, t4>>16, t4&0xFFFF);
  printf("sp=0x%04X_%04X  t5=0x%04X_%04X\n", sp>>16, sp&0xFFFF, t5>>16, t5&0xFFFF);
  printf("fp=0x%04X_%04X  t6=0x%04X_%04X\n", fp>>16, fp&0xFFFF, t6>>16, t6&0xFFFF);
  printf("ra=0x%04X_%04X  t7=0x%04X_%04X\n", ra>>16, ra&0xFFFF, t7>>16, t7&0xFFFF);

  uint32_t status = GetReg(REG_RX_RISC_STATUS);
  if (status & REG_RX_RISC_STATUS__HARDWARE_BREAKPOINT)
    printf("Exception: Hardware Breakpoint\n");
  if (status & REG_RX_RISC_STATUS__HALT_INSTRUCTION_EXECUTED)
    printf("Exception: Halt Instruction Executed\n");
  if (status & REG_RX_RISC_STATUS__INVALID_INSTRUCTION)
    printf("Exception: Invalid Instruction\n");
  if (status & REG_RX_RISC_STATUS__PAGE_0_DATA_REFERENCE)
    printf("Exception: Page 0 Data Reference\n");
  if (status & REG_RX_RISC_STATUS__PAGE_0_INSTRUCTION_REFERENCE)
    printf("Exception: Page 0 Instruction Reference\n");
  if (status & REG_RX_RISC_STATUS__INVALID_DATA_ACCESS)
    printf("Exception: Invalid Data Access\n");
  if (status & REG_RX_RISC_STATUS__INVALID_INSTRUCTION_FETCH)
    printf("Exception: Invalid Instruction Fetch\n");
  if (status & REG_RX_RISC_STATUS__BAD_MEMORY_ALIGNMENT)
    printf("Exception: Bad Memory Alignment\n");
  if (status & REG_RX_RISC_STATUS__MEMORY_ADDRESS_TRAP)
    printf("Exception: Memory Address Trap\n");
  if (status & REG_RX_RISC_STATUS__REGISTER_ADDRESS_TRAP)
    printf("Exception: Register Address Trap\n");
  if (status & REG_RX_RISC_STATUS__BIT_11)
    printf("Exception: Bit 11\n");
  if (status & REG_RX_RISC_STATUS__BIT_12)
    printf("Bit 12\n");
  if (status & REG_RX_RISC_STATUS__BIT_13)
    printf("Bit 13\n");
  if (oldStatus & REG_RX_RISC_STATUS__HALTED)
    printf("Condition: Halted\n");
  if (status & REG_RX_RISC_STATUS__DATA_ACCESS_STALL)
    printf("Condition: Data Access Stall\n");
  if (status & REG_RX_RISC_STATUS__INSTRUCTION_FETCH_STALL)
    printf("Condition: Instruction Access Stall\n");
  if (status & REG_RX_RISC_STATUS__BLOCKING_READ)
    printf("Condition: Blocking Read\n");

  if (!(oldMode & REG_RX_RISC_MODE__HALT))
    SetReg(REG_RX_RISC_MODE, oldMode);

  return 0;
}

static int _UsageDumpNVM(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: ");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "<offset> <length>\n");
  fprintf(stderr,
    "  Dump contents of NVM to stdout, starting at byte <offset>\n"
    "  and for <length> bytes. Hex notation supported. Binary data\n"
    "  will be written to stdout. Length is rounded up to a multiple\n"
    "  of four bytes. If length exceeds size of NVM device, the extra\n"
    "  output bytes have undefined contents.\n"
    );
  return -2;
}

static int _CmdDumpNVM(int pargc, int argc, char **argv) {
  if (argc < 3)
    return _UsageDumpNVM(pargc, argc, argv);

  uint32_t offset = 0;
  uint32_t len    = 0;
  char *tail = NULL;

  offset = strtoul(argv[1], &tail, 0);
  if (!tail || tail == argv[1] || *tail)
    return _UsageDumpNVM(pargc, argc, argv);

  len    = strtoul(argv[2], &tail, 0);
  if (!tail || tail == argv[2] || *tail)
    return _UsageDumpNVM(pargc, argc, argv);

  if (len % 4)
    len += 4-(len%4);

  for (; len; offset += 4, len -= 4) {
    uint32_t data = NVMRead32(offset, ARB_ACQUIRE|ARB_RELEASE);
    data = ntohl(data);
    if (fwrite(&data, sizeof(data), 1, stdout) < 1)
      return -1;
  }

  return 0;
}

static int _UsageRestoreNVM(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: ");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "<filename> <offset>\n");
  fprintf(stderr,
    "  Flash the contents of a file into the device NVM, starting\n"
    "  at byte offset <offset> and continuing until the end of the\n"
    "  file is reached. Hex notation supported. The file size and\n"
    "  <offset> must be a multiple of four bytes.\n"
    "\n"
    "  CAUTION: Backup device contents before modifying data:\n"
    "    sudo ethtool -e <devname> raw on > backup.bin\n"
    );
  return -2;
}

static int _CmdRestoreNVM(int pargc, int argc, char **argv) {
  if (argc != 3)
    return _UsageRestoreNVM(pargc, argc, argv);
  
  const char *filename = argv[1];
  uint32_t offset = 0;
  char *tail = NULL;

  offset = strtoul(argv[2], &tail, 0);
  if (!tail || tail == argv[2] || *tail)
    return _UsageRestoreNVM(pargc, argc, argv);

  if (offset % 4) {
    fprintf(stderr, "error: offset given is not 32-bit aligned\n");
    return -1;
  }

  FILE *f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "error: couldn't open file\n");
    return -1;
  }

  if (fseek(f, 0, SEEK_END))
    return -1;

  if (ftell(f) % 4) {
    fprintf(stderr, "error: file size is not a multiple of four bytes\n");
    return -1;
  }

  if (fseek(f, 0, SEEK_SET))
    return -1;

  uint32_t buf[256];
  uint32_t bytesSet = 0;
  uint32_t bytesWritten = 0;
  for (;;) {
    size_t rd = fread(buf, sizeof(uint32_t), ARRAYLEN(buf), f);
    if (rd < 1)
      break;

    for (size_t i=0; i<rd; ++i)
      buf[i] = htonl(buf[i]);

    for (size_t i=0; i<rd; ++i) {
      uint32_t curValue = NVMRead32(offset, ARB_ACQUIRE|ARB_RELEASE);
      // Avoid writing values which are already correct to avoid unnecessary
      // wear on the NVM device.
      if (curValue != buf[i]) {
        printf("would set [0x%X] = 0x%08X (currently 0x%08X)\n", offset, buf[i], curValue);
        NVMWrite32(offset, buf[i], ARB_ACQUIRE|ARB_RELEASE);
        bytesWritten += 4;
      }

      bytesSet += 4;
      offset += 4;
    }
  }

  fprintf(stderr, "%u bytes set (%u written, %u already correct, write factor %u%%)\n", bytesSet, bytesWritten, bytesSet-bytesWritten,
    (bytesWritten*100)/bytesSet);
  return 0;
}

static int _UsageSetNVM(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: ");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "<offset> <value>\n");
  fprintf(stderr,
    "  Write a single 32-bit word <value> to NVM at a given byte <offset>.\n"
    "  The <offset> must be 32-bit aligned. Hex notation supported.\n"
    "\n"
    "  CAUTION: Backup device contents before modifying data:\n"
    "    sudo ethtool -e <devname> raw on > backup.bin\n"
    );
  return -2;
}

static int _CmdSetNVM(int pargc, int argc, char **argv) {
  if (argc < 3)
    return _UsageSetNVM(pargc, argc, argv);

  uint32_t offset = 0;
  uint32_t value = 0;
  char *tail = NULL;

  offset = strtoul(argv[1], &tail, 0);
  if (!tail || tail == argv[1] || *tail)
    return _UsageSetNVM(pargc, argc, argv);

  value  = strtoul(argv[2], &tail, 0);
  if (!tail || tail == argv[2] || *tail)
    return _UsageSetNVM(pargc, argc, argv);

  if (offset % 4) {
    fprintf(stderr, "error: offset must be 32-bit aligned\n");
    return 1;
  }

  NVMWrite32(offset, value, ARB_ACQUIRE|ARB_RELEASE);
  return 0;
}

static int _UsageGetReg(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: ");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "<regno>\n");
  fprintf(stderr,
    "  Output the value of a device register. <regno> is the\n"
    "  register offset. Hex notation is supported. The output\n"
    "  is a 32-bit unsigned value in hex notation.\n"
    );
  return -2;
}

static int _CmdGetReg(int pargc, int argc, char **argv) {
  if (argc < 2)
    return _UsageGetReg(pargc, argc, argv);

  uint32_t regno = 0;
  char *tail = NULL;

  regno = strtoul(argv[1], &tail, 0);
  if (!tail || tail == argv[1] || *tail)
    return _UsageGetReg(pargc, argc, argv);

  printf("0x%08X\n", GetReg(regno));

  return 0;
}

static int _UsageGetMII(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: ");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "[port] <regno>\n");
  fprintf(stderr,
    "  Output the value of a given MII register. <regno> is the\n"
    "  register number. Hex notation is supported. The output\n"
    "  is a 16-bit unsigned value in hex notation. If [port] is\n"
    "  omitted, uses the PHY port which would normally be used\n"
    "  by the given device function. Alternatively, specify [port]\n"
    "  as an integer, or as \"gphy\" or \"serdes\".\n"
    );
  return -2;
}

static int _CmdGetMII(int pargc, int argc, char **argv) {
  if (argc < 2 || argc > 3)
    return _UsageGetMII(pargc, argc, argv);

  uint32_t portno = 0;
  uint32_t regno = 0;
  char *tail = NULL;

  if (argc == 3) {
    if (!strcmp(argv[1], "gphy"))
      portno = GetGPHYNo();
    else if (!strcmp(argv[1], "serdes"))
      portno = GetSERDESNo();
    else {
      portno = strtoul(argv[1], &tail, 0);
      if (!tail || tail == argv[1] || *tail)
        return _UsageGetMII(pargc, argc, argv);
    }
    ++argv;
  } else
    portno = GetPHYNo();

  regno = strtoul(argv[1], &tail, 0);
  if (!tail || tail == argv[1] || *tail || portno > UINT8_MAX || regno > UINT8_MAX)
    return _UsageGetMII(pargc, argc, argv);

  printf("%02x[%02x] = 0x%04X\n", portno, regno, MIIRead((uint8_t)portno, (uint8_t)regno));

  return 0;
}

static int _UsageSetMII(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: ");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "[port] <regno> <value>\n");
  fprintf(stderr,
    "  Set the value of a given MII register. <regno> is the\n"
    "  register number. Hex notation is supported. The <value>\n"
    "  is a 16-bit unsigned value. If [port] is omitted, uses\n"
    "  the PHY port which would normally be used by the given\n"
    "  device function. Alternatively, specify [port] as an\n"
    "  integer, or as \"gphy\" or \"serdes\".\n"
    );
  return -2;
}

static int _CmdSetMII(int pargc, int argc, char **argv) {
  if (argc < 3 || argc > 4)
    return _UsageSetMII(pargc, argc, argv);

  uint32_t portno = 0;
  uint32_t regno = 0;
  uint32_t value = 0;
  char *tail = NULL;

  if (argc == 4) {
    if (!strcmp(argv[1], "gphy"))
      portno = GetGPHYNo();
    else if (!strcmp(argv[1], "serdes"))
      portno = GetSERDESNo();
    else {
      portno = strtoul(argv[1], &tail, 0);
      if (!tail || tail == argv[1] || *tail)
        return _UsageGetMII(pargc, argc, argv);
    }
    ++argv;
  } else
    portno = GetPHYNo();

  regno = strtoul(argv[1], &tail, 0);
  if (!tail || tail == argv[1] || *tail)
    return _UsageSetMII(pargc, argc, argv);

  value = strtoul(argv[2], &tail, 0);
  if (!tail || tail == argv[2] || *tail ||
      portno > UINT8_MAX || regno > UINT8_MAX || value > UINT16_MAX)
    return _UsageSetMII(pargc, argc, argv);

  MIIWrite((uint8_t)portno, (uint8_t)regno, (uint16_t)value);
  return 0;
}

static int _UsageDumpROM(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: ");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "\n");
  fprintf(stderr,
    "  Dumps the device ROM to stdout. This is a disruptive process\n"
    "  that involves resetting the device a lot.\n"
    );
  return -2;
}

static int _CmdDumpROM(int pargc, int argc, char **argv) {
  if (argc != 1)
    return _UsageDumpROM(pargc, argc, argv);

  _CmdHalt(pargc, argc, argv);

  for (uint32_t addr=0x40000000; addr < 0x40004400; addr += 4) {
    SetReg(REG_RX_RISC_PROGRAM_COUNTER, addr);
    uint32_t inst = htonl(GetReg(REG_RX_RISC_CUR_INSTRUCTION));
    size_t wr = fwrite(&inst, sizeof(inst), 1, stdout);
    if (wr < 1)
      return 1;
  }

  return 0;
}

static int _UsageBootmem(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: [-w] <otg.bin>");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "\n");
  fprintf(stderr,
    "Boot a device image by loading it into memory and jumping to it.\n"
    "This command doesn't write to NVM, so it can be used to debug\n"
    "firmware images without writing them to the NVM and wearing it out.\n"
    "\n"
    "The image must be designed to support memory boot (built with OTG_DEBUG)\n"
    "in order for the stage2 to boot properly.\n"
    "\n"
    "Pass a full NVM image. The stage1/stage2 will be extracted and loaded into\n"
    "memory automatically, and the device RX RISC will be reset to its entrypoint.\n"
    "\n"
    "If -w is passed, the debug output protocol will be attached, meaning that\n"
    "the bootcode (if built with debug support) will block on log output to\n"
    "'otgdbg tail'. This ensures you don't miss any output between running\n"
    "'otgdbg bootmem' and 'otgdbg tail', but will cause the bootcode to block\n"
    "forever until 'otgdbg tail' is first run. Note that exiting 'otgdbg tail'\n"
    "always detaches the debug output protocol, meaning that future output\n"
    "will not block on the host receiving it.\n"
    "\n"
    "If -w is not passed, any debug output produced between running 'otgdbg bootmem'\n"
    "and 'otgdbg tail' is discarded.\n"
    );
  return -2;
}

static int _CmdBootmem(int pargc, int argc, char **argv) {
  if (argc < 2)
    return _UsageBootmem(pargc, argc, argv);

  bool attachLog = false;
  if (!strcmp(argv[1], "-w")) {
    attachLog = true;
    ++argv;
  }

  int fd = open(argv[1], O_RDONLY|O_SYNC);
  if (fd < 0) {
    fprintf(stderr, "error: couldn't open file\n");
    return -1;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    fprintf(stderr, "error: couldn't stat file\n");
    return -1;
  }

  void *virt = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (!virt) {
    fprintf(stderr, "error: couldn't mmap file\n");
    return -1;
  }

  otg_header *hdr = (otg_header*)virt;
  if (st.st_size < sizeof(otg_header)) {
    fprintf(stderr, "error: short file\n");
    return -1;
  }

  if (ntohl(hdr->magic) != HEADER_MAGIC) {
    fprintf(stderr, "error: file lacks correct magic\n");
    return -1;
  }

  uint32_t s1ImageBase = ntohl(hdr->s1Entrypoint);
  uint32_t s1ImageSizeInWords = ntohl(hdr->s1Size);
  uint32_t s1ImageSize = s1ImageSizeInWords*4;
  uint32_t s1ImageOffset = ntohl(hdr->s1Offset);

  if ((s1ImageOffset + s1ImageSize + sizeof(otg_s2header)) > st.st_size) {
    fprintf(stderr, "error: file is too short\n");
    return -1;
  }

  otg_s2header *s2hdr = (otg_s2header*)((uint8_t*)virt + s1ImageOffset + s1ImageSize);
  if (ntohl(s2hdr->magic) != HEADER_MAGIC) {
    fprintf(stderr, "error: stage2 lacks correct magic\n");
    return -1;
  }

  uint32_t s2ImageSize = ntohl(s2hdr->s2Size);

  if ((s1ImageOffset + s1ImageSize + sizeof(otg_s2header) + s1ImageSize) > st.st_size) {
    fprintf(stderr, "error: file is too short\n");
    return -1;
  }

  OrReg(REG_RX_RISC_MODE, REG_RX_RISC_MODE__HALT);
  SetReg(REG_FAST_BOOT_PROGRAM_COUNTER, 0);
  SetReg(REG_RX_RISC_STATUS, REG_RX_RISC_STATUS__CLEARABLE_MASK);

  uint32_t offset = s1ImageOffset;
  for (uint32_t addr = s1ImageBase; addr < (s1ImageBase+s1ImageSize); addr += 4, offset += 4)
    SetRXWord(addr, ntohl(*(uint32_t*)((uint8_t*)virt + offset)));

  offset += sizeof(otg_s2header);
  for (uint32_t addr = 0x08000000; addr < (0x08000000+s2ImageSize); addr += 4, offset += 4)
    SetRXWord(addr, ntohl(*(uint32_t*)((uint8_t*)virt + offset)));

  SetGencom32(0x350, 0xDECAFBAD);
  SetGencom32(0x354, s1ImageSizeInWords);
  SetGencom32(0x358, s1ImageOffset);
  SetGencom32(0x35C, s2ImageSize);
  SetGencom32(0x364, 0);
  SetGencom32(0x368, attachLog ? 0xDECAFBAD : 0);

  SetReg(REG_RX_RISC_PROGRAM_COUNTER, s1ImageBase);
  MaskReg(REG_RX_RISC_MODE, REG_RX_RISC_MODE__HALT);
  return 0;
}

static int _UsageBootAPE(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: <ape.bin>");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "\n");
  return -2;
}


static int _CmdBootAPE(int pargc, int argc, char **argv) {
  if (argc < 2)
    return _UsageBootAPE(pargc, argc, argv);

  int fd = open(argv[1], O_RDONLY|O_SYNC);
  if (fd < 0) {
    fprintf(stderr, "error: couldn't open file\n");
    return -1;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    fprintf(stderr, "error: couldn't stat file\n");
    return -1;
  }

  void *virt = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (!virt) {
    fprintf(stderr, "error: couldn't mmap file\n");
    return -1;
  }

  OrReg(REG_APE__MODE, REG_APE__MODE__HALT|REG_APE__MODE__FAST_BOOT);

  // Make it easy to tell if boot succeeded.
  SetReg(REG_APE__SEG_SIG, 0x1122BABA);
  SetReg(REG_APE__APEDBG_CMD, REG_APE__APEDBG_CMD__MAGIC|REG_APE__APEDBG_CMD__TYPE__RETURN);
  SetReg(REG_APE__APEDBG_STATE, 0x00222200);
  SetReg(REG_APE__APEDBG_CMD,   0);
  SetReg(REG_APE__APEDBG_ARG0,  0);
  SetReg(REG_APE__APEDBG_ARG1,  0);
  SetReg(REG_APE__APEDBG_CMD_ERROR_FLAGS,  0);
  SetReg(REG_APE__APEDBG_EXCEPTION_COUNT,  0);

  uint32_t *words = virt;
  size_t numWords = st.st_size/4;
  uint32_t addr = 0x10D800;
  for (size_t i=0; i<numWords; ++i)
    SetAPEWord(addr + i*4, words[i]);

  SetReg(REG_APE__GPIO_MSG, 0xD800|0x100002);
  MaskReg(REG_APE__MODE, REG_APE__MODE__HALT);
  OrReg(REG_APE__MODE, REG_APE__MODE__RESET);

  return 0;
}

static int _UsageBootAPEShell(int pargc, int argc, char **argv) {
  fprintf(stderr, "usage: <ape_shell.bin>");
  PrintCommand(pargc, argc, argv);
  fprintf(stderr, "\n");
  return -2;
}


static int _CmdBootAPEShell(int pargc, int argc, char **argv) {
  if (argc < 2)
    return _UsageBootAPE(pargc, argc, argv);

  int fd = open(argv[1], O_RDONLY|O_SYNC);
  if (fd < 0) {
    fprintf(stderr, "error: couldn't open file\n");
    return -1;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    fprintf(stderr, "error: couldn't stat file\n");
    return -1;
  }

  if (st.st_size > 0x8000) {
    fprintf(stderr, "error: oversized shellcode\n");
    return -1;
  }

  void *virt = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (!virt) {
    fprintf(stderr, "error: couldn't mmap file\n");
    return -1;
  }

  if (GetReg(REG_APE__SEG_SIG) != APE_APE_MAGIC) {
    fprintf(stderr, "error: APE is not running\n");
    return -1;
  }

  if (!(GetReg(REG_APE__FW_STATUS) & REG_APE__FW_STATUS__READY)) {
    fprintf(stderr, "error: APE is not ready\n");
    return -1;
  }

  fprintf(stderr, "setting apedbg cmd 1\n");
  SetReg(REG_APE__APEDBG_CMD, REG_APE__APEDBG_CMD__MAGIC|REG_APE__APEDBG_CMD__TYPE__RETURN);

  uint32_t scratchpadBase  = 0x00100000;

  struct { uint32_t injectionVector, expectedCode; } knownVectors[] = {
    { 0x1033B2, 0x2000B5F0, }, // push {r4-r7,lr}; movs r0, #0
    { 0x1033E8, 0x41F0E92D, }, // push.w {r4-r8,lr}
    {},
  };

  uint32_t injectionVector;
  for (size_t i=0;; ++i) {
    injectionVector = knownVectors[i].injectionVector;
    if (!injectionVector) {
      fprintf(stderr, "error: cannot find any known injection vector\n");
      return -1;
    }

    uint32_t word = GetAPEEventScratchpadWord(injectionVector - scratchpadBase);
    if (word == knownVectors[i].expectedCode)
      break;
  }

  fprintf(stderr, "setting apedbg state\n");
  SetReg(REG_APE__APEDBG_STATE, 0x00222200);
  fprintf(stderr, "setting apedbg cmd\n");
  SetReg(REG_APE__APEDBG_CMD,   0);
  fprintf(stderr, "setting apedbg arg0\n");
  SetReg(REG_APE__APEDBG_ARG0,  0);
  fprintf(stderr, "setting apedbg arg1\n");
  SetReg(REG_APE__APEDBG_ARG1,  0);
  fprintf(stderr, "setting apedbg error flags\n");
  SetReg(REG_APE__APEDBG_CMD_ERROR_FLAGS,  0);
  fprintf(stderr, "setting apedbg exception count\n");
  SetReg(REG_APE__APEDBG_EXCEPTION_COUNT,  0);

  fprintf(stderr, "writing apedbg shellcode\n");

  // Write shellcode to some area we don't care about.
  uint32_t *words = virt;
  size_t numWords = (st.st_size+3)/4;
  uint32_t imageBase = scratchpadBase;
  for (size_t i=0; i<numWords; ++i)
    SetAPEEventScratchpadWord(imageBase + i*4 - scratchpadBase, words[i]);

  // 0x6022_0260  APEDBG_STATE
  //
  // Patch function. We use the following sequence of instructions:
  //   push {r4,lr}         10 B5  (0xB510)
  //   ldr  r4, _addr       01 4C  (0x4C01)
  //   blx  r4              A0 47  (0x47A0)
  //   pop  {r4,pc}         10 BD  (0xBD10)
  // _addr:
  //   .long TARGET_ADDR|1  ...4 bytes...
  uint32_t patch[] = {
    0x4C01B510,
    0xBD1047A0,

    //0xB5104C01, // push {r4,lr}; ldr r4, _addr
    //0x47A0BD10, // blx r4; pop {r4, pc}
    
#ifdef __ppc64__
    // This is what we must use on ppc64le:
    imageBase|1,
#else
    // This is what we must use on x86:
    (imageBase|1)<<16,
    (imageBase|1)>>16,
#endif
    // I do not understand why this is.
  };

  fprintf(stderr, "patching function\n");
  for (size_t i=0; i<ARRAYLEN(patch); ++i)
    SetAPEEventScratchpadWord(injectionVector - scratchpadBase + i*4, htole32(patch[i]));

#if 0
  uint32_t buf[100];
  for (size_t i=0; i<100; ++i)
    buf[i] = le32toh(GetAPEEventScratchpadWord(injectionVector - scratchpadBase + i*4));

  fwrite(buf, 4, 100, stdout);
#endif

  fprintf(stderr, "sending ape event\n");
  if (LockAPEEvent()) {
    fprintf(stderr, "error: timeout when trying to lock APE event system\n");
    return -1;
  }

  // Set the flag so that the INT 0x02 Event Type 0x0B handler will call the
  // function.
  //OrReg(REG_APE__NCSI_DBGLOG_HOUSEKEEPING, 0x40000);
  OrReg(REG_APE__NCSI_UNK2C, 0x40000);

  SetReg(REG_APE__EVENT_STATUS,
    REG_APE__EVENT_STATUS__DRIVER_EVENT
   |REG_APE__EVENT_STATUS__TYPE__0B
   |REG_APE__EVENT_STATUS__PENDING);

  UnlockAPEEvent();

  SetReg(REG_APE__EVENT, REG_APE__EVENT__1);
  if (WaitAPEEvent()) {
    fprintf(stderr, "error: timeout waiting for APE event to complete\n");
    return -1;
  }

  fprintf(stderr, "done\n");
  return 0;
}

static bool g_stopTail = false;

static void _SigInt(int signo) {
  g_stopTail = true;
}

static int _CmdTail(int pargc, int argc, char **argv) {
  SetGencom32(0x368, 0xDECAFBAD);
  signal(SIGHUP, _SigInt);
  signal(SIGINT, _SigInt);
  signal(SIGQUIT, _SigInt);

  for (;;) {
    uint32_t logData, logStatus, logByte;

    for (;;) {
      if (g_stopTail)
        goto stop;

      logStatus = GetGencom32(0x364);
      if (logStatus)
        break;
      sched_yield();
    }

    logData = SwapEndian32(GetGencom32(0x360));
    logByte = logData & 0xFF;
    if (logByte)
      fputc(logByte, stdout);
    logByte = (logData >> 8) & 0xFF;
    if (logByte)
      fputc(logByte, stdout);
    logByte = (logData >> 16) & 0xFF;
    if (logByte)
      fputc(logByte, stdout);
    logByte = (logData >> 24) & 0xFF;
    if (logByte)
      fputc(logByte, stdout);

    SetGencom32(0x364, 0);
  }

stop:
  SetGencom32(0x368, 0);
  return 1;
}

#define SPLIT(X) ((X) >> 16), ((X) & 0xFFFF)
#define HEX32 "0x%04X_%04X"

static int _CmdAPEInfo(int pargc, int argc, char **argv) {
  // apeinfo -r

  uint32_t ape0   = GetReg(APE_REG(0x000));
  uint32_t ape4   = GetReg(APE_REG(0x004));

  printf("[000] Mode: " HEX32 "\n", SPLIT(ape0));
  printf("  MemECC=%u\n",       !!(ape0 & 0x00040000));
  printf("  PCIeF1=%u\n",       !!(ape0 & 0x00020000));
  printf("  SMBGlitchFil=%u\n", !!(ape0 & 0x00010000));
  printf("  LATQ0=%u\n",        !!(ape0 & 0x00008000));
  printf("  LARQ0=%u\n",        !!(ape0 & 0x00004000));
  printf("  LATQ1=%u\n",        !!(ape0 & 0x00800000));
  printf("  LARQ1=%u\n",        !!(ape0 & 0x00400000));
  printf("  GRCint=%u\n",       !!(ape0 & 0x80000000));
  printf("  Event2=%u\n",       !!(ape0 & 0x00000040));
  printf("  Event1=%u\n",       !!(ape0 & 0x00000020));
  printf("  HostDiag=%u\n",     !!(ape0 & 0x00000008));
  printf("  FastBoot=%u\n",     !!(ape0 & 0x00000004));
  printf("  IcodePipRd=%u\n",   !!(ape0 & 0x00080000));
  printf("  rvrsARBbyte=%u\n",  !!(ape0 & 0x00001000));
  printf("  swapARBdw=%u\n",    !!(ape0 & 0x00000800));
  printf("  rvrsATBbyte=%u\n",  !!(ape0 & 0x00000400));
  printf("  swapATBdw=%u\n",    !!(ape0 & 0x00000200));
  printf("  halt=%u\n",         !!(ape0 & 0x00000002));
  printf("  reset=%u\n",        !!(ape0 & 0x00000001));

  uint32_t ape2C = GetReg(APE_REG(0x02C));
  printf("[02C] Mode 2: " HEX32 "\n", SPLIT(ape2C));
  printf("  LATQ2=%u\n",        !!(ape2C & 0x00008000));
  printf("  LARQ2=%u\n",        !!(ape2C & 0x00004000));
  printf("  LATQ3=%u\n",        !!(ape2C & 0x00800000));
  printf("  LARQ3=%u\n",        !!(ape2C & 0x00400000));

  static const char *const _apeStatus1[16] = {
    "",
    "NMI Exception",
    "Fault Exception",
    "Memory Check",
    "Unknown 4",
    "Romloader Disabled",
    "Magic Number",
    "APE Init Code",
    "Header Checksum",
    "APE Header",
    "Image Checksum",
    "NVRAM Checksum",
    "Invalid Type",
    "ROM Loader Checksum",
    "Invalid Unzip Len",
    "Unknown F",
  };
  static const char *const _apeStatus2[16] = {
    "Prog 0",
    "Prog 1",
    "BPC Enter",
    "Decode",
    "Read NVRAM Header",
    "Read Code",
    "Jump",
    "Prog 7",
    "BPC Success",
  };

  printf("[004] Status: " HEX32 "\n", SPLIT(ape4));
  printf("  Boot: %u (%s)\n", ape4 >> 28, _apeStatus1[ape4 >> 28]);
  printf("        %u (%s)\n", (ape4 >> 24) & 0xF, _apeStatus2[(ape4 >> 24) & 0xF]);

  printf("  LAN 0 Dstate: %s\n",
    (ape4 & 0x010) ? "D3" : "D0-2");
  printf("  LAN 1 Dstate: %s\n",
    (ape4 & 0x200) ? "D3" : "D0-2");
  printf("  Boot: %s\n",
    (ape4 & 0x020) ? "Fast" : "NVRAM");

  printf("  Occurred Flags:\n");
  printf("    NVRAM Control Reset=%u\n",        !!(ape4 & 0x00000008));
  printf("    LAN Dstate Change=%u\n",          !!(ape4 & 0x00000004));
  printf("    LAN GRC Reset=%u\n",              !!(ape4 & 0x00000002));
  printf("    PCIe Reset=%u\n",                 !!(ape4 & 0x00000001));
  printf("    LAN 1 GRC Reset=%u\n",            !!(ape4 & 0x00000080));
  printf("    LAN 1 Dstate Change=%u\n",        !!(ape4 & 0x00000100));

  uint32_t ape30 = GetReg(APE_REG(0x030));
  printf("[030] Status 2: " HEX32 "\n", SPLIT(ape30));
  printf("  LAN 2 Dstate: %s\n",
    (ape30 & 0x010) ? "D3" : "D0-2");
  printf("  LAN 3 Dstate: %s\n",
    (ape30 & 0x200) ? "D3" : "D0-2");

  printf("  Occurred Flags:\n");
  printf("    LAN 2 GRC Reset=%u\n",            !!(ape30 & 0x00000002));
  printf("    LAN 2 Dstate Change=%u\n",        !!(ape30 & 0x00000004));
  printf("    LAN 3 GRC Reset=%u\n",            !!(ape30 & 0x00000080));
  printf("    LAN 3 Dstate Change=%u\n",        !!(ape30 & 0x00000100));

  static const char *const _apeCPUStatus[16] = {
    "Running",
    "Halted",
    "Locked Out",
    "Sleeping",
    "Deep Sleep",
    "Res5",
    "Res6",
    "Res7",
    "Int Pending",
    "Int Entry",
    "Int Exit",
    "Int Return",
    "Res12",
    "Res13",
    "Res14",
    "Res15",
  };

  uint32_t ape108 = GetReg(APE_REG(0x108));
  printf("[108] CM3: " HEX32 "\n", SPLIT(ape108));
  printf("  CPU: %u (%s)\n", ape108 & 0x0F, _apeCPUStatus[ape108 & 0x0F]);
  printf("  Active Interrupt No.=%u\n", (ape108 & 0xFF00) >> 8);

  uint32_t apeB8  = GetReg(APE_REG(0x0B8));
  printf("[0B8] GPIO: " HEX32 "\n", SPLIT(apeB8));
  //TODO
  
  uint32_t apeBC  = GetReg(APE_REG(0x0BC));
  printf("[0BC] GInt: " HEX32 "\n", SPLIT(apeBC));
  // TODO


  return 0;
}

static int _CmdAPELog(int pargc, int argc, char **argv) {
  uint32_t idx = GetReg(REG_APE__NCSI_DBGLOG_INDEX)+1;

  for (size_t i=0; i<APE_DBGLOG_NUM_ENTRIES; ++i) {
    uint32_t typeArg = GetReg(APE_DBGLOG_BASE + 8*((idx+1 + i) % APE_DBGLOG_NUM_ENTRIES) + 0);
    uint32_t ts      = GetReg(APE_DBGLOG_BASE + 8*((idx+1 + i) % APE_DBGLOG_NUM_ENTRIES) + 4);

    uint8_t type     = typeArg & 0xFF;
    uint32_t arg     = typeArg >> 8;
    char buf[4] = "   ";
    if (type >= 0x20 && type <= 0x7E) {
      buf[0] = buf[2] = '\'';
      buf[1] = type;
    }

    printf("%2zu)  [%9u r%01X] type=0x%02X (%s)  arg=0x  %02X_%04X",
      (idx+1 + i) % APE_DBGLOG_NUM_ENTRIES,
      ts & 0x0FFFFFFF, ts>>28,
      typeArg & 0xFF, buf,
      (typeArg>>24)&0xFF, (typeArg>>8) & 0xFFFF);

    switch (type) {
      case 0x14:
        printf("  INT 0x14 General Status Change:");
        if (arg & 0x000001) printf(" PCIereset");
        if (arg & 0x000002) printf(" P0_GRCrst");
        if (arg & 0x000004) printf(" P0_DsChg");
        if (arg & 0x000080) printf(" P1_GRCrst");
        if (arg & 0x000100) printf(" P1_DsChg");
        if (arg & 0x002000) printf(" P2_GRCrst");
        if (arg & 0x004000) printf(" P2_DsChg");
        if (arg & 0x100000) printf(" P3_GRCrst");
        if (arg & 0x080000) printf(" P3_DsChg");
        break;

      case 0x18:
        // Note: The bit tested here is actually from bit 19 of
        // REG_MISCELLANEOUS_CONFIGURATION. Which is listed as "PME_EN_State".
        // Does this bit mean Vmain?
        printf("  INT 0x18: enter %s", (arg & 0x080000) ? "Vmain" : "Vaux");
        break;

      // LINK STATUS CHANGE
      // ------------------
      case 0x19: // Port 0, Interrupt 0x19 -- Link Status Change (Even Ports)
      case 0x1A: // Port 1, Interrupt 0x1A -- Link Status Change (Odd Ports)
      case 0x41: // Port 2, Interrupt 0x19 -- Link Status Change (Even Ports)
      case 0x42: // Port 3, Interrupt 0x1A -- Link Status Change (Odd Ports)
      {
        static const char *const _str[] = {"1000mb", "100mb", "10mb", "nolnk"};
        uint8_t intNo, portNo;
        switch (type) {
          case 0x19:
            portNo = 0;
            intNo = 0x19;
            break;
          case 0x1A:
            portNo = 1;
            intNo = 0x1A;
            break;
          case 0x41:
            portNo = 2;
            intNo = 0x19;
            break;
          case 0x42:
            portNo = 3;
            intNo = 0x1A;
            break;
        }

        printf("  INT 0x%02X Link Status Change: port%u D%u ps=%u ms=%u lnk=%s %s",
          intNo, portNo,
          (arg >>  8) & 0x3,
          (arg >>  4) & 0x7,
          (arg      ) & 0xF,
          _str[(arg >> 19) & 0x3],
          (arg & 0x02000) ? "Vmain" : "Vaux"
          );
      } break;

      // RX PACKET
      // ---------
      case 0x0B: // Port 0, Interrupt 0x0B -- RX Packet (Even Ports)
      case 0x1B: // Port 1, Interrupt 0x1B -- RX Packet (Odd Ports)
      case 0x33: // Port 2, Interrupt 0x0B -- RX Packet (Even Ports)
      case 0x43: // Port 3, Interrupt 0x1B -- RX Packet (Odd Ports)
      {
        uint8_t intNo, portNo;
        switch (type) {
          case 0x0B:
            portNo = 0;
            intNo = 0x0B;
            break;
          case 0x1B:
            portNo = 1;
            intNo = 0x1B;
            break;
          case 0x33:
            portNo = 2;
            intNo = 0x0B;
            break;
          case 0x43:
            portNo = 3;
            intNo = 0x1B;
            break;
        }
        printf("  INT 0x%02X RX Packet: port%u head=0x%02X tail=0x%02X bufCount=0x%X",
          intNo, portNo,
          (arg >> 8) & 0xFF, arg & 0xFF, (arg>>18) & 0xF);
      } break;

      case 0xDA:
        printf("  fault: chip1 0x%06x", arg);
        break;

      case 0xE0: {
        uint32_t subtype = (arg >> 16) & 0xFF;
        char typebuf[64];
        const char *type;
        switch (subtype) {
          default:
            snprintf(typebuf, sizeof(typebuf), "unknown subtype: BrcmOem.0x%02X", subtype);
            type = typebuf;
            break;
        }
        printf("  %s", type);
      } break;
      case 0xF0: {
        uint32_t subtype = (arg >> 16) & 0xFF;
        char typebuf[64];
        const char *type;
        switch (subtype) {
          default:
            snprintf(typebuf, sizeof(typebuf), "unknown subtype: 0x%02X", subtype);
            type = typebuf;
            break;
        }
        printf("  %s", type);
      } break;
      case 0xF4: {
        printf("  FIX: p%02x", arg & 0xFF);
        if (arg & 0x002000) printf("  ARPMerr");
        if (arg & 0x004000) printf("  ARPMresetted");
        if (arg & 0x008000) printf("  RegRepair");
        if (arg & 0x080000) printf("  PhyBusy");
        if (arg & 0x100000) printf("  MacResetted");
        if (arg & 0x200000) printf("  MacPhyMismatch");
        if (arg & 0x400000) printf("  DriverBusy");
        if (arg & 0x800000) printf("  DriverDead");
      } break;
      default:
        break;
    }

    printf("\n");
  }

//
// Types:
//   24 (0x18)  "int24: enter V(main|aux)" (arg bit 0x08_0000 means Vmain, else Vaux)
//   20 (0x14)  "int20: "
//     arg bit 0x00_0001: "PCIEreset"
//     arg bit 0x00_0004: "P0_DsChg"
//     arg bit 0x00_0002: "P0_GRCrst"
//     arg bit 0x00_0100: "P1_DsChg"
//     arg bit 0x00_0080: "P1_GRCrst"
//     arg bit 0x00_4000: "P2_DsChg"
//     arg bit 0x00_2000: "P2_GRCrst"
//     arg bit 0x10_0000: "P3_DsChg"
//     arg bit 0x08_0000: "P3_GRCrst"
//
//  LINK STATUS CHANGE
//  ------------------
//    Interrupt 0x19: Link Status Change for Ports 0 (Type 0x19), 2 (Type 0x41)
//    Interrupt 0x1A: Link Status Change for Ports 1 (Type 0x1A), 3 (Type 0x42)
//
//  25 (0x19):  "int25: p0 D$x ps=$y ms=$z lnk=$w $s"
//     where $x = logArg bits 8-9
//           $y = logArg bits 4-6
//           $z = logArg bits 0-3
//           $w = based on bits 19-20:
//             0: "1000mb", 1: "100mb", 2: "10mb", 3: "nolnk"
//           $s = "Vmain" if logArg bit 0x0_2000 is set, "Vaux" otherwise.
//  26 (0x1A):  "int26: p1 D$x ps=$y ms=$z lnk=$w $s"
//    Same format and encoding as above.
//  65 (0x41):  "int25: p2 D$x ps=$y ms=$z lnk=$w $s"
//    Same format and encoding as above.
//  66 (0x42):  "int26: p3 D$x ps=$y ms=$z lnk=$w $s"
//    Same format and encoding as above.
//
//  RX PACKET
//  ---------
//    Interrupt 0x0B: RX Packet for Ports 0 (Type 0x0B), 2 (Type 0x33)
//    Interrupt 0x1B: RX Packet for Ports 1 (Type 0x1B), 3 (Type 0x43)
//
//  11 (0x0B):  "int11: p0 rx pkt head=$head tail=$tail bufCnt=$bufCnt"
//    $head = logArg bits 8-15
//    $tail = logArg bits 0-7
//    $bufCnt = logArg bits 18-21
//  27 (0x1B):  "int27: p1 rx pkt head=$head tail=$tail bufCnt=$bufCnt"
//    Same format and encoding as above.
//  51 (0x33):  "int11: p2 rx pkt head=$head tail=$tail bufCnt=$bufCnt"
//    Same format and encoding as above.
//  67 (0x43):  "int27: p3 rx pkt head=$head tail=$tail bufCnt=$bufCnt"
//    Same format and encoding as above.
//  17 (0x11):  "int17: rmu egress $x $y saHit:$saHit len:$len $z"
//    $x = if logArg bit 0x02, "pt", otherwise "cmd"
//    $y = if logArg bit 0x01, "bad", otherwise ""
//    $saHit = if logArg bit 0x04, bits 9-12, otherwise (-1)
//    $len = bits 13-23
//    $z = if logArg bit 0x08, "vlan", otherwise ""
//  8 (0x08):  "int13: h2b $Xlwords $y$z"
//    $X = logArg bits 2-23
//    $y = if logArg bit 0x02, "underflow", else ""
//    $z = if logArg bit 0x01, "!empty", else ""
//  0xE0, 0xE1, 0xF0:
//    "$x_$s (ch:$y/$z) $w"
//    $x = logArg 16-23
//    $s = based on logArg 16-23:
//      if log type is 0xF0:
//        0:  "ClrInitState"
//        1:  "SelPkg"
//        2:  "DeSelPkg"
//        3:  "EnableCh"
//        4:  "DisableCh"
//        5:  "ResetCh"
//        6:  "EnableChTx"
//        7:  "DisableChTx"
//        8:  "EnableAEN"
//        9:  "SetLnk"
//       10:  "GetLnkStatus"
//       11:  "SetVlanFilt"
//       12:  "EnableVlan"
//       13:  "DisableVlan"
//       14:  "SetMacAddr"
//       15:  ?
//       16:  "EnBcastFilt"
//       17:  "DisBcastFilt"
//       18:  "EnMcastFilt"
//       19:  "DisMcastFilt"
//       20:  "SetNcsiFc"
//       21:  "GetVerId"
//       22:  "GetCaps"
//       23:  "GetParams"
//       24:  "GetChPktStat"
//       25:  "GetNcsiStat"
//       26:  "GetPtStat"
//       80:  "OEM"
//       else: "invalid"
//     if log type is 0xE0:
//       0:   "BrcmOem.SetVirMac"
//       1:   "BrcmOem.GetNcsiParam"
//       5:   "BrcmOem.GetTempRead"
//       else: "invalid"
//     if log type is 0xE1:
//       -1:   "DellOem.GetInventory"
//       1:   "DellOem.GetExtCap"
//       2:   "DellOem.GetPartInfo"
//       3:   "DellOem.GetFcoeCap"
//       4:   "DellOem.GetVirLink"
//       5:   "DellOem.GetLanStat"
//       6:   "DellOem.GetFcoeStat"
//       7:   "DellOem.SetAddr"
//       8:   "DellOem.GetAddr"
//       9:   "DellOem.SetLicense"
//      10:   "DellOem.GetLicense"
//      11:   "DellOem.SetPtCtrl"
//      12:   "DellOem.GetPtCtrl"
//      13:   "DellOem.SetPartTxBw"
//      14:   "DellOem.GetPartTxBw"
//      15:   ?
//      16:   "DellOem.SetMcIpAddr"
//      17:   "DellOem.GetTeamInfo"
//      18:   "DellOem.DisablePort"
//      19:   "DellOem.GetTemp"
//      20:   "DellOem.SetLinkTune"
//      21:   "DellOem.EnableOobWoL"
//      22:   "DellOem.DisOobWoL"
//      else: "invalid"
//  $y = logArg bits 13-23 (hex format)
//  $z = logArg bits 8-12  (hex format)
//  $w = based on logArb bits 0-7:
//     0: "OK"
//     1: "ErrInstanceId"
//     2: "ErrCmdType"
//     3: "ErrPayloadLen"
//     4: "ErrChksum"
//     5: "ErrInitReq"
//     6: "ErrPkgId"
//     7: "ErrChId"
//     8: "ErrSysBusy"
//     9: "OemOK"
//    10: "ErrOemIana"
//    11: "ErrOemCmdType"
//    12: "ErrOemPkgPayloadLen"
//    13: "ErrOemCmdPayloadLen"
//    14: "ErrHdrRev"
//   128: "RMU->SMbus"
//   129: "SMBus->RMU"
//   else: print as hex ("%02x_???")
//  0xEE: "card: $x°C Tdead(g3):$y,$z TfanOnAux(g4):$w,$a"
//    todo
//    $x = if logArg bit 0x80, "N2B", else "H2B"
//    $y = logArg bits 0-6
//    $z = logArg bits 16-23 (hex)
//    $w = logArg bits 8-15 (hex)
//    $a =
//      if logArg bit 0x1_0000 and logArg bit 0x0_0100, "n2b"
//      if logArg bit 0x1_0000 but not logArg bit 0x0_0100, "_n2b"
//      if not logArg bit 0x1_0000 but logArg bit 0x0_0100, "+n2b"
//      else ""
//    $b =
//      if logArg bit 0x2_0000 and logArg bit 0x0_0200, "b2n"
//      if logArg bit 0x2_0000 and not logArg bit 0x0_0200, "_b2n"
//      if not logArg bit 0x2_0000 and logArg bit 0x0_0200, "+b2n"
//      else ""
//    $c =
//      if logArg bit 0x4_0000 and logArg bit 0x0_0400, "h2b"
//      if logArg bit 0x4_0000 and not logArg bit 0x0_0400, "_h2b"
//      if not logArg bit 0x4_0000 and logArg bit 0x0_0400, "+h2b"
//      else ""
//    $d =
//      if logArg bit 0x8_0000 and logArg bit 0x0_0800, "b2h"
//      if logArg bit 0x8_0000 and not logArg bit 0x0_0800, "_b2h"
//      if not logArg bit 0x8_0000 and logArg bit 0x0_0800, "+b2h"
//      else ""
//  0xF5: "!failed rsps=$x reason=$y-$z"
//    $x = bits 16-23 (hex)
//    $y = bits 0-7   (hex)
//    $z = bits 8-15  (hex)
//  0xF1: "aen: "
//    bits 0-7:
//      0: LinkChg
//      1: CfgReq
//      2: DrvrChg
//
//    todo
//  0xF2: "!!!: crashed PC=$x"
//    $x: logArg (hex)
//  0xF3:
//    if bits 8-23 are nonzero,
//      "drv: p$x present $y.$z"
//    else "drv: p$x absent"
//    $x = bits 0-7
//    $y = bits 16-23
//    $z = bits 8-15
//  0xF6: "drv: p$x "
//    $x = bits 0-7
//    bits 16-23:
//      0x01: "DIAG"
//      0x10: "ODI"
//      0x12: "UNDI"
//      0x13: "UEFI"
//      0xF0: "LINUX"
//      0xF4: "SOLARIS"
//      0xF6: "FREEBSD"
//      0xF8: "NETWARE"
//      0xF1,0xF2,0xF3,0xF5,0xF7,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF: print hex
//      other: "NDIS" + print hex
//    bits 8-15:
//      1: "start"
//      2: "unload"
//      3: "WOL"
//      4: "suspend"
//  0xF4: "fix: p$x"
//    $x = bits 0-7
//    bit 0x002000: "ARPMerr"
//    bit 0x004000: "ARPMresetted"
//    bit 0x008000: "RegRepair"
//    bit 0x080000: "PhyBusy"       -- generated if MIIWait times out, port number in low bits
//    bit 0x100000: "MacResetted"
//    bit 0x200000: "MacPhyMismatch"
//    bit 0x400000: "DriverBusy"
//    bit 0x800000: "DriverDead"
//  0xF7: "mctpCtrl:Rq$x D$y $z $w_"
//    $x = bit 7
//    $y = bit 6
//    $z = bits 0-4
//    $w = bits 8-15
//    based on $w:
//      1: "SetEndpointID"
//      2: "GetEndpointID"
//      4: "GetMCTPVerSupport"
//      5: "GetMsgTypeSupport"
//    if bit 7 is NOT set:
//      based on bits 16-23:
//        0: "SUCCESS"
//        1: "ERROR"
//        2: "InvalidData"
//        3: "InvalidLen"
//        4: "NotReady"
//        5: "UnsupportedCmd"
//  0xD8: "card: $x.$y°C"
//    todo
//  0xDB: "VPD: update chip$x $y version"
//    $x = bits 8-15
//    $y = if bits 0-7 are nonzero, "IMFW", else "BOOT"
//  0xD9: "fault: chip0 $x"
//    $x = bits 0-23
//  0xDA: "fault: chip1 $x"
//    $x = bits 0-23
//  0xDC: "gpio: ..."
//    todo

  return 0;
}

static int _CmdAPECrash(int pargc, int argc, char **argv) {
  if (GetReg(REG_APE__CRE_SEG_SIG) != CRE_MAGIC) {
    printf("no crash info detected\n");
    return 1;
  }

  static const char *const _vectorNames[] = {
    [ 1] = "Reset",
    [ 2] = "NMI",
    [ 3] = "Hard Fault",
    [ 4] = "Memory Management",
    [ 5] = "Bus Fault",
    [ 6] = "Usage Fault",
    [11] = "SVCall",
    [12] = "Debug Monitor",
    [14] = "PendSV",
    [15] = "SysTick",
  };

  printf("Exception Count:  %u\n", GetReg(REG_APE__CRE_COUNT));
  printf("Last Heartbeat:   0x%08X   Last Ticks:         0x%08X\n",
    GetReg(REG_APE__CRE_LAST_HEARTBEAT),
    GetReg(REG_APE__CRE_LAST_TICKS));
  printf("Last FW Status:   0x%08X   Current FW Status:  0x%08X\n", GetReg(REG_APE__CRE_LAST_FW_STATUS),
    GetReg(REG_APE__FW_STATUS));
  printf("Exception Flags:  0x%08X\n", GetReg(REG_APE__CRE_EXCEPTION_FLAGS));
  printf("APEDBG:\n");
  printf("  State:                0x%08X\n", GetReg(REG_APE__APEDBG_STATE));
  printf("  Command Error Flags:  0x%08X\n", GetReg(REG_APE__APEDBG_CMD_ERROR_FLAGS));
  printf("  Exception Count:      0x%08X\n", GetReg(REG_APE__APEDBG_EXCEPTION_COUNT));

  printf("\n");
  uint32_t vec = GetReg(REG_APE__CRE_EXCEPTION_VECTOR);
  const char *vecStr = "?";
  if (vec < ARRAYLEN(_vectorNames) && _vectorNames[vec])
    vecStr = _vectorNames[vec];
  printf("Exception Vector: 0x%08X (%s)\n", vec, vecStr);
  printf("R0:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R0)));
  printf("R1:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R1)));
  printf("R2:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R2)));
  printf("R3:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R3)));
  printf("R4:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R4)));
  printf("R5:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R5)));
  printf("R6:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R6)));
  printf("R7:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R7)));
  printf("R8:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R8)));
  printf("R9:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R9)));
  printf("R10:   " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R10)));
  printf("R11:   " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R11)));
  printf("R12:   " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_R12)));
  printf("LR:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_LR)));
  printf("PC:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_PC)));
  printf("xPSR:  " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_XPSR)));
  printf("SP:    " HEX32 "\n",         SPLIT(GetReg(REG_APE__CRE_REG_SP)));

  return 0;
}

int _CmdAPEReset(int pargc, int argc, char **argv) {
  MaskOrReg(REG_APE__MODE, REG_APE__MODE__FAST_BOOT, REG_APE__MODE__HALT);

  for (size_t port=0; port<8; ++port) {
    uint32_t state = GetReg(REG_APE__PER_LOCK_GRANT + port*4);
    if (state) {
      fprintf(stderr, "warning: lockport %zu has bits 0x%08X, clearing\n", port, state);
      SetReg(REG_APE__PER_LOCK_GRANT + port*4, state);
    }
  }

  SetReg(REG_APE__SEG_SIG, 0);
  SetReg(REG_APE__EVENT_STATUS, 0);
  SetReg(REG_APE__NCSI_SIG, 0);
  SetReg(REG_APE__FW_STATUS, 0);
  SetReg(REG_APE__APEDBG_STATE, 0);
  MaskReg(REG_APE__NCSI_UNK2C, 0x40000);

  MaskOrReg(REG_APE__MODE, REG_APE__MODE__HALT, REG_APE__MODE__RESET);
  return 0;
}

typedef struct {
  const char *name;
  const char *tagline;
  int (*func)(int pargc, int argc, char **argv);
  bool anyDevice;
} command_def_t;

static const command_def_t _commands[] = {
  {.name = "info",
   .func = _CmdInfo,
   .tagline = "Show device info. Can be used on any PCI device, not just supported devices.",
   .anyDevice = true,
  },
  {.name = "get",
   .tagline = "Get contents of device memory.",
   .func = _CmdGet,
  },
  {.name = "dump",
   .tagline = "Get contents of device memory (binary output).",
   .func = _CmdDump,
  },
  {.name = "set",
   .tagline = "Sets a device word",
   .func = _CmdSet,
  },
  {.name = "halt",
   .tagline = "Halts the RX RISC",
   .func = _CmdHalt,
  },
  {.name = "resume",
   .tagline = "Resumes the RX RISC",
   .func = _CmdResume,
  },
  {.name = "step",
   .tagline = "Single-steps the RX RISC",
   .func = _CmdStep,
  },
  {.name = "dumpnvm",
   .tagline = "Dumps contents of device NVM.",
   .func = _CmdDumpNVM,
  },
  {.name = "setnvm",
   .tagline = "Sets an NVM word.",
   .func = _CmdSetNVM,
  },
  {.name = "restorenvm",
   .tagline = "Restores contents of device NVM.",
   .func = _CmdRestoreNVM,
  },
  {.name = "getmii",
   .tagline = "Gets a device port MII register.",
   .func = _CmdGetMII,
  },
  {.name = "setmii",
   .tagline = "Sets a device port MII register.",
   .func = _CmdSetMII,
  },
  {.name = "dumprom",
   .tagline = "Dumps a device's RX CPU boot ROM.  WARNING: will reset the device.\n",
   .func = _CmdDumpROM,
  },
  {.name = "cpuinfo",
   .tagline = "Outputs current state of CPU registers.",
   .func = _CmdCPUInfo,
  },
  {.name = "bootmem",
   .tagline = "Boot a firmware image via memory (doesn't touch NVM)",
   .func = _CmdBootmem,
  },
  {.name = "bootape",
   .tagline = "Experimental",
   .func = _CmdBootAPE,
  },
  {.name = "bootapeshell",
   .tagline = "Run APE shellcode",
   .func = _CmdBootAPEShell,
  },
  {.name = "tail",
   .tagline = "Stream out OTG debug log from device",
   .func = _CmdTail,
  },
  {.name = "apeinfo",
   .tagline = "APE info",
   .func = _CmdAPEInfo,
  },
  {.name = "apelog",
   .tagline = "APE debug log",
   .func = _CmdAPELog,
  },
  {.name = "apecrash",
   .tagline = "Show APE crash info",
   .func = _CmdAPECrash,
  },
  {.name = "apereset",
   .tagline = "Reset APE",
   .func = _CmdAPEReset,
  },
};

int main(int argc, char **argv) {
  int ec;

  if (argc < 3) {
    fprintf(stderr, "usage: %s <pci-bus-addr> <command> <args...>\n", argv[0]);
    fprintf(stderr, "commands:\n");
    for (size_t i=0; i<ARRAYLEN(_commands); ++i)
      fprintf(stderr, "  %12s    %s\n", _commands[i].name, _commands[i].tagline ? _commands[i].tagline : "");
    return 1;
  }

  ec = DeviceResolveByString(argv[1], &g_devInfo);
  if (ec < 0)
    return 1;

  ec = DeviceLoadInfo(&g_devInfo);
  if (ec < 0)
    return 1;

  const command_def_t *cmd = NULL;
  for (size_t i=0; i<ARRAYLEN(_commands); ++i)
    if (!strcmp(_commands[i].name, argv[2])) {
      cmd = &_commands[i];
      break;
    }

  if (!cmd) {
    fprintf(stderr, "error: unknown command: \"%s\"\n", argv[2]);
    ec = 1;
    goto error;
  }

  g_devInfo.type = DeviceDetermineSupport(&g_devInfo, _supportTable);
  if (g_devInfo.type < 0 && !cmd->anyDevice) {
    fprintf(stderr, "error: unsupported device\n");
    ec = 1;
    goto error;
  }

  ec = MMIOOpen(g_devInfo.resources[0].physStart, g_devInfo.resources[0].physEnd-g_devInfo.resources[0].physStart+1, &g_mmio12);
  if (ec < 0)
    return 1;

  ec = MMIOOpen(g_devInfo.resources[2].physStart, g_devInfo.resources[2].physEnd-g_devInfo.resources[2].physStart+1, &g_mmio34);
  if (ec < 0)
    return 1;

  g_bar12 = g_mmio12->virt;
  g_bar34 = g_mmio34->virt;

  _IssueWarnings();

  ec = cmd->func(2, argc-2, argv+2);

error:
  MMIOClose(g_mmio12);
  MMIOClose(g_mmio34);
  return ec;
}
