#include "otg.h"
#ifdef OTG_HOST
#  include <unistd.h>
#endif

static const uint8_t g_apePortTable[] = {APE_PORT_PHY0, APE_PORT_PHY1, APE_PORT_PHY2, APE_PORT_PHY3}; //0,2,3,5

static void Sleep(uint32_t delay) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

// Wait for the MII communication interface to not be busy.
static void MIIWait(void) {
#ifdef PROPRIETARY
/*               scrubbed                    */
#endif
}

// Read an MDI value via the MII communication interface.
// phyNo and regNo are 5 bits.
static uint16_t MIIRead(uint8_t phyNo, uint8_t regNo) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

// Begin writing an MDI value via the MII communication interface. Does not
// wait for the write to finish, but does wait for any previous writes to
// finish first. Use MIIWait afterwards if synchronous operation is required.
static void MIIWrite(uint8_t phyNo, uint8_t regNo, uint16_t data) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

static void MIIOr(uint8_t phyNo, uint8_t regNo, uint16_t bitsToOr) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

// Mask out a bit of a MII register and rewrite it.
static void MIIMask(uint8_t phyNo, uint8_t regNo, uint16_t bitsToDisable) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

// Sets or clears a bit of a MII register and rewrites it.
static void MIISetClear(uint8_t phyNo, uint8_t regNo, uint16_t bit, bool yes) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

// This is only used with the SERDES PHYs - the GPHYs have a different register
// map with a different method of overloading the limited MII register space,
// so never try and use it on a GPHY.
static void MIISelectBlockSERDES(uint8_t phyNo, uint16_t blockNo) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

static void NVMAcquire(void) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

static uint32_t NVMAcquireAndRelease(void) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

static void NVMWaitDone(void) {
#ifdef PROPRIETARY
/*               scrubbed                    */
#endif
}

// TODO: refactor
static uint32_t NVMGetPageSize(void) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

#if defined(STAGE1)
extern uint32_t g_using264BytePagesForNVM;

static uint32_t NVMInflateByteCountIfUsing264BytePages(uint32_t byteOffset) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

#elif defined(STAGE2) || defined(OTG_HOST)
static uint32_t NVMInflateByteCountIfUsing264BytePages(uint32_t byteOffset) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}
#else
#  error STAGE1/STAGE2 not defined
#endif

// Takes flags ARB_ACQUIRE, ARB_RELEASE.
static uint32_t NVMRead32(uint32_t byteOffset, uint32_t flags) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

#ifdef OTG_HOST
static void NVMReadBulk(uint32_t byteOffset, void *buf, size_t numWords, uint32_t flags) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

static void NVMWrite32(uint32_t byteOffset, uint32_t value, uint32_t flags) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

// untested
static void NVMWriteBulk(uint32_t byteOffset, const void *buf, uint32_t numWords, uint32_t flags) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}
#endif

static uint32_t NVMFindEntry(uint8_t desiredTag) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

// len is in words.
static uint32_t ComputeCRC(const void *base, uint32_t len, uint32_t initialValue) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

static void DebugPrint(const char *msg) {
#ifdef OTG_HOST
  puts(msg);
#else
  for (;*msg;) {
    uint32_t x = 0;
    uint8_t c;
    for (size_t i=0; i<4; ++i) {
      c = *msg;
      if (c) {
        x <<= 8;
        x |= c;
        ++msg;
      }
    }

    SetGencom32(0x360, x);
    SetGencom32(0x364, 1);
    while (GetGencom32(0x364))
      if (GetGencom32(0x368) != 0xDECAFBAD)
        return;
  }
#endif
}


#ifdef OTG_HOST
static bool g_readFail = false;

static uint32_t _SetReadFailure(const char *msg) {
  if (!g_readFail) {
    fprintf(stderr, "WARNING: Read failure occurred. Further failures will not be indicated, and failed reads will return 0xFFFF_FFFF. Cause of first failure: %s\n", msg);
    g_readFail = true;
  }

  return 0xFFFFFFFF;
}

// General APE Locking
// ---------------------------------------------------------------------------

// lockType: APE_PORT_*
static uint32_t GetAPELockBit(uint32_t lockType) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

// lockType: APE_PORT_*
static int LockAPE(uint32_t lockType) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

// lockType: APE_PORT_*
static void UnlockAPE(uint32_t lockType) {
#ifdef PROPRIETARY
/*               scrubbed                    */
#endif
}

// APE Event System
// ---------------------------------------------------------------------------

static int WaitAPEEvent(void) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

static int LockAPEEvent(void) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

static void UnlockAPEEvent(void) {
#ifdef PROPRIETARY
/*               scrubbed                    */
#endif
}

static void _WarnAboutAPEState(const char *msg) {
  static bool _done = false;
  if (_done)
    return;

  fprintf(stderr, "warning: APE is not in correct state for scratchpad read/write to work; reads/writes will fail (%s)\n", msg);
  _done = true;
}

static uint32_t GetAPEEventScratchpadWord(uint32_t offset) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

void SetAPEEventScratchpadWord(uint32_t offset, uint32_t value) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

static void _WarnAboutAPEStateShell(const char *msg) {
  static bool _done = false;
  if (_done)
    return;

  fprintf(stderr, "warning: apedbg is not running, shell reads/writes will not work (%s)\n", msg);
  _done = true;
}

static int _WaitAPEShellCmd(void) {
  uint32_t timeout = 10000/10;
  while (GetReg(REG_APE__APEDBG_CMD)) {
    if (!timeout--) {
      _WarnAboutAPEState("command timed out");
      return -1;
    }

    usleep(10);
  }

  return 0;
}

uint32_t GetAPEMemShell(uint32_t offset) {
  if (GetReg(REG_APE__APEDBG_STATE) != REG_APE__APEDBG_STATE__RUNNING) {
    _WarnAboutAPEState("apedbg not running");
    return 0xFFFFFFFF;
  }

  if (_WaitAPEShellCmd())
    return 0xFFFFFFFF;

  SetReg(REG_APE__APEDBG_ARG0, offset);
  SetReg(REG_APE__APEDBG_CMD,
    REG_APE__APEDBG_CMD__MAGIC
   |REG_APE__APEDBG_CMD__TYPE__MEM_GET);

  if (_WaitAPEShellCmd())
    return 0xFFFFFFFF;

  uint32_t exc = GetReg(REG_APE__APEDBG_CMD_ERROR_FLAGS);
  if (exc) {
    _WarnAboutAPEState("exception caught");
    return 0xFFFFFFFF;
  }

  return GetReg(REG_APE__APEDBG_ARG1);
}

void SetAPEMemShell(uint32_t offset, uint32_t v) {
  if (GetReg(REG_APE__APEDBG_STATE) != REG_APE__APEDBG_STATE__RUNNING) {
    _WarnAboutAPEState("apedbg not running");
    return;
  }

  if (_WaitAPEShellCmd())
    return;

  SetReg(REG_APE__APEDBG_ARG0, offset);
  SetReg(REG_APE__APEDBG_ARG1, v);
  SetReg(REG_APE__APEDBG_CMD,
    REG_APE__APEDBG_CMD__MAGIC
   |REG_APE__APEDBG_CMD__TYPE__MEM_SET);

  _WaitAPEShellCmd();

  uint32_t exc = GetReg(REG_APE__APEDBG_CMD_ERROR_FLAGS);
  if (exc)
    _WarnAboutAPEState("exception caught");
}

// OTP Access
// ---------------------------------------------------------------------------
static uint32_t GetOTP(uint32_t offset) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

// Compression
// ---------------------------------------------------------------------------
#define CMPS_MAGIC 0x53504D43

typedef struct __attribute__((packed)) {
  uint32_t magic;             // "CMPS" (CMPS_MAGIC)
  uint32_t compressedSize;    //
  uint32_t uncompressedSize;
  uint32_t unkC;
} cmps_header;
static_assert(sizeof(cmps_header) == 16, "cmps_header");

typedef struct {
  uint8_t dict[2048    +64];
  size_t dictIdx;
} cmps_state;

static cmps_state g_state;

// uncompSize may be SIZE_MAX
typedef ssize_t (write_t)(uint8_t ch, void *arg);
static void Decompress(const void *inStart, size_t inBytes, size_t uncompSize, write_t *writef, void *arg,
    size_t *bytesRead_, size_t *bytesWritten_) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

#endif
