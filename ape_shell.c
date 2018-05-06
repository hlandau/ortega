#include <stdint.h>

#define CONSTP32(X)    (*(uint32_t*)(X))
#define CONSTPP(X)     (*(void**)(X))

#define APEDBG_STATE CONSTP32(0x60220260)
#define APEDBG_CMD   CONSTP32(0x60220264)
#define APEDBG_ARG0  CONSTP32(0x60220268)
#define APEDBG_ARG1  CONSTP32(0x6022026C)
#define APEDBG_CMD_ERROR_FLAGS             CONSTP32(0x60220270)
#define APEDBG_CMD_ERROR_FLAGS__EXCEPTION  0x00000001
#define APEDBG_EXCEPTION_COUNT             CONSTP32(0x60220274)
#define APEDBG_EXCEPTION_IGNORE            CONSTP32(0x60220278)

#define APEDBG_STATE_RUNNING    0xBEEFCAFE
#define APEDBG_STATE_EXITED     0xBEEF0FFE

#define APEDBG_CMD_TYPE_MASK   0x0000FFFF
#define APEDBG_CMD_MAGIC_MASK  0xFFFF0000
#define APEDBG_CMD_MAGIC       0x5CAF0000

#define APE_EVENT_STATUS CONSTP32(0x60220300)

#define NVIC_VECTOR_TABLE_OFFSET  CONSTPP(0xE000ED08)
#define INT_HARD_FAULT   3

enum {
  APEDBG_CMD__NONE          = 0x0000,
  APEDBG_CMD__MEM_GET       = 0x0001,
  APEDBG_CMD__MEM_SET       = 0x0002,
  APEDBG_CMD__CALL_0        = 0x0003,
  APEDBG_CMD__RETURN        = 0x0004,
};

typedef struct {
  uint32_t r4, r5, r6, r7, r8, r9, r10, r11;
  uint32_t lr, r12, r0, r1, r2, r3, xpsr, pc;
} exc_frame_t;

extern void APEShell_IntHandler_HardFault(void);
void _IntHandler_HardFault(exc_frame_t *sp) {
  APEDBG_CMD_ERROR_FLAGS |= APEDBG_CMD_ERROR_FLAGS__EXCEPTION;
  ++APEDBG_EXCEPTION_COUNT;

  if (APEDBG_EXCEPTION_IGNORE) {
    sp->pc += 2;
    APEDBG_EXCEPTION_IGNORE = 0;
  }
}

static inline uint32_t _GetCommand(void) {
  for (;;) {
    APEDBG_STATE = APEDBG_STATE_RUNNING;

    uint32_t cmd = APEDBG_CMD;
    if ((cmd & APEDBG_CMD_TYPE_MASK)
        && (cmd & APEDBG_CMD_MAGIC_MASK) == APEDBG_CMD_MAGIC) {
      APEDBG_CMD &= ~APEDBG_CMD_MAGIC_MASK;
      return cmd & APEDBG_CMD_TYPE_MASK;
    }
  }
}

static inline void _CommandComplete(void) {
  APEDBG_CMD = 0;
}

// APE debugging agent, injected into APE memory via shellcode.
void APEShellStart(void) {
  uint32_t status;
  asm volatile ("mrs %0, PRIMASK" : "=r" (status));
  asm volatile ("cpsid i");

  APE_EVENT_STATUS = 0;
  APEDBG_CMD_ERROR_FLAGS = 0;
  APEDBG_EXCEPTION_IGNORE = 0;
  APEDBG_CMD = 0;

  void **nvicTable = (void**)NVIC_VECTOR_TABLE_OFFSET;
  void *oldHardFaultHandler = nvicTable[INT_HARD_FAULT];
  nvicTable[INT_HARD_FAULT] = APEShell_IntHandler_HardFault;

  // Process commands.
  for (;;) {
    uint32_t cmd = _GetCommand();

    APEDBG_CMD_ERROR_FLAGS = 0;
    switch (cmd) {
      case APEDBG_CMD__MEM_GET:
        APEDBG_EXCEPTION_IGNORE = 1;
        APEDBG_ARG1 = *(uint32_t*)APEDBG_ARG0;
        APEDBG_EXCEPTION_IGNORE = 0;
        break;

      case APEDBG_CMD__MEM_SET:
        APEDBG_EXCEPTION_IGNORE = 1;
        *(uint32_t*)APEDBG_ARG0 = APEDBG_ARG1;
        APEDBG_EXCEPTION_IGNORE = 0;
        break;

      case APEDBG_CMD__CALL_0: {
        uint32_t addr = APEDBG_ARG0;
        APEDBG_ARG0 = 0;

        typedef uint32_t (*funcp0_t)(void);
        APEDBG_ARG1 = ((funcp0_t)addr)();
      } break;

      case APEDBG_CMD__RETURN:
        _CommandComplete();
        nvicTable[INT_HARD_FAULT] = oldHardFaultHandler;
        APEDBG_STATE = APEDBG_STATE_EXITED;
        asm ("msr PRIMASK, %0" :: "r" (status));
        return;

      default:
        break;
    }

    _CommandComplete();
  }
}

asm(
  ".pushsection .textstart,\"ax\",%progbits\n"
  ".global APEShellEntrypoint\n"
  ".code 16\n"
  ".thumb_func\n"
  "APEShellEntrypoint:\n"
  "  b APEShellStart\n"
  "\n"
  ".global APEShell_IntHandler_HardFault\n"
  ".thumb_func\n"
  "APEShell_IntHandler_HardFault:\n"
  "  push {lr}\n"
  "  push {r4-r11}\n"
  "  mov r0, sp\n"
  "  bl _IntHandler_HardFault\n"
  "  pop {r4-r11}\n"
  "  pop {pc}\n"
  "  \n"
  "\n"
  ".popsection\n"
  );

/*
asm(
  ".pushsection .trailer,\"ax\",%progbits\n"
  ".global APETrailer\n"
  ".code 16\n"
  ".thumb_func\n"
  "APETrailer:\n"
  "  push {r4,lr}\n"
  "  ldr r4, APETarget\n"
  "  blx  r4\n"
  "  pop {r4,pc}\n"
  "APETarget:\n"
  "  .4byte APEShellEntrypoint\n"
  ".popsection\n"
  );*/
