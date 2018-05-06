#include "otg.h"
#include <stdint.h>
#include <stdnoreturn.h>

/* Image Build Information
 * -----------------------
 */
#define APE_IMAGE_NAME "OTG NCSI"
#define APE_VER_MAJOR 1
#define APE_VER_MINOR 3
#define APE_VER_PATCH 7

/* Miscellaneous Defines
 * ---------------------
 */
#define SCRATCHTEXT    __attribute__((section(".scratchtext")))
#define INTERRUPT      __attribute__((interrupt))
#define NO_INLINE      __attribute__((noinline))

#define _STRINGIFY(X) #X
#define STRINGIFY(X) _STRINGIFY(X)

// Originally these used dates/times in the format "Dec  3 2014" and "15:25:28"
// respectively. We don't use dates/times since this needlessly affects build
// determinism. We use something else meaningful instead. Note that these strings
// must remain exactly 11 and 8 characters long, respectively.
#define BUILD_DATE    "OTG OTG OTG"
#define BUILD_TIME    "HAMSTROK"
static_assert(sizeof(BUILD_DATE) == (11+1), "build date");
static_assert(sizeof(BUILD_TIME) == ( 8+1), "build time");

/* Memory Map
 * ----------
 */
#define STACK_END                                 0x00118000

#define APE_DEVREG_BASE                           0xA0040000
#define APE_APEREG_BASE                           0x60200000
#define APE_SHM_BASE                              0x60220000
#define APE_PERIPHERAL_BASE                       0x60240000
#define APE_NVM_BASE                              (APE_PERIPHERAL_BASE+0x0000)
#define APE_SMBUS1_BASE                           (APE_PERIPHERAL_BASE+0x1000)
 
#define NVIC_INT_CONTROL_TYPE                     0xE000E004
#define NVIC_SYSTICK_CONTROL                      0xE000E010
#define NVIC_SYSTICK_RELOAD_VALUE                 0xE000E014
#define NVIC_SYSTICK_CUR_VALUE                    0xE000E018
#define NVIC_SYSTICK_CALIBRATION_VALUE            0xE000E01C
#define NVIC_INT_ENABLE                           0xE000E100
#define NVIC_INT_DISABLE                          0xE000E180
#define NVIC_INT_SET_PENDING                      0xE000E200
#define NVIC_INT_CLEAR_PENDING                    0xE000E280
#define NVIC_INT_ACTIVE_MASK                      0xE000E300
#define NVIC_INT_PRIORITY                         0xE000E400
#define NVIC_CPUID                                0xE000ED00
#define NVIC_INT_CONTROL                          0xE000ED04
#define NVIC_INT_CONTROL__EXCEPTION_VECTOR__MASK  0x000000FF
#define NVIC_TABLE_OFFSET                         0xE000ED08
#define NVIC_APP_INT_CONTROL                      0xE000ED0C
#define NVIC_SYS_CONTROL                          0xE000ED10
#define NVIC_CFG_CONTROL                          0xE000ED14
#define NVIC_HANDLER_PRIORITY_4                   0xE000ED18
#define NVIC_HANDLER_PRIORITY_8                   0xE000ED18
#define NVIC_HANDLER_PRIORITY_12                  0xE000ED18
#define NVIC_HANDLER_CONTROL                      0xE000ED24
#define NVIC_CONFIG_FAULT_STATUS                  0xE000ED28
#define NVIC_HARD_FAULT_STATUS                    0xE000ED2C
#define NVIC_DEBUG_FAULT_STATUS                   0xE000ED30
#define NVIC_MEM_MANAGE_ADDR                      0xE000ED34
#define NVIC_BUS_FAULT_ADDR                       0xE000ED38
#define NVIC_AUX_FAULT_STATUS                     0xE000ED3C
#define NVIC_SW_TRIG_INT                          0xE000EF00

// The stack frame which the CPU generates automatically when entering an
// interrupt.
typedef struct {
  uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;
} isr_args;
//typedef struct {
//  uint32_t r4, r5, r6, r7, r8, r9, r10, r11;
//  uint32_t lr, r12, r0, r1, r2, r3, xpsr, pc;
//} exc_frame_t;
 
// Used to get the registers which the CPU doesn't save automatically, if one
// should need them for some reason. Call immediately at the start of a
// function tagged with INTERRUPT.
#define ISR_GET_REGS()                                                        \
  uint32_t r4,r5,r6,r7,r8,r9,r10,r11;                                         \
  asm (                                                                       \
    "mov %0, r4\n"                                                            \
    "mov %1, r5\n"                                                            \
    "mov %2, r6\n"                                                            \
    "mov %3, r7\n"                                                            \
    "mov %4, r8\n"                                                            \
    "mov %5, r9\n"                                                            \
    "mov %6, r10\n"                                                           \
    "mov %7, r11\n"                                                           \
  : "=r"(r4), "=r"(r5), "=r"(r6), "=r"(r7),                                   \
    "=r"(r8), "=r"(r9), "=r"(r10), "=r"(r11))                              /**/

typedef void (isr_t)(isr_args *args);

// Disable interrupts. Returns the old PRIMASK (representing which interrupts
// were masked locally).
static inline uint32_t DisableInterrupts(void) {
  uint32_t mask;
  asm volatile (
    "mrs %0, PRIMASK\n"
    "cpsid i\n"
    :"=r" (mask));
  return mask;
}

// Reenable interrupts by setting PRIMASK to mask. Specifying a mask of 0
// enables all interrupts. Generally you want to pass the value previously
// returned by DisableInterrupts to restore the previous state of PRIMASK.
static inline void EnableInterrupts(uint32_t mask) {
  asm volatile ("msr PRIMASK, %0" :: "r" (mask));
}

// The ISR table. The Cortex-M3's NVIC automatically uses this table to jump to
// the right ISR when an interrupt is processed.
typedef union {
  struct {
    void *stackEnd;
    isr_t *reset;
    isr_t *nmi;
    isr_t *hardFault;
    isr_t *memManage;
    isr_t *busFault;
    isr_t *usageFault;
    isr_t *reserved01C[4];
    isr_t *svCall;
    isr_t *debugMon;
    isr_t *reserved034;
    isr_t *pendSV;
    isr_t *sysTick;
    isr_t *ext[32];
  };
  void *slots[48];
} isr_table;

enum {
  INT_RESET       =  1,
  INT_NMI         =  2,
  INT_HARD_FAULT  =  3,
  INT_MEM_MANAGE  =  4,
  INT_BUS_FAULT   =  5,
  INT_USAGE_FAULT =  6,
  INT_SVCALL      = 11,
  INT_DEBUGMON    = 12,
  INT_PENDSV      = 14,
  INT_SYSTICK     = 15,
  INT_EXT0        = 16,
};

// WARNING: These are numbered relative to INT_EXT0.
enum {
  EXTINT_HANDLE_EVENT                  = 0x02,
  EXTINT_03H                           = 0x03,
  EXTINT_H2B                           = 0x08,
  EXTINT_0AH                           = 0x0A,
  EXTINT_RX_PACKET_EVEN_PORTS          = 0x0B,
  EXTINT_RMU_EGRESS                    = 0x11,
  EXTINT_GEN_STATUS_CHANGE             = 0x14,
  EXTINT_VOLTAGE_SOURCE_CHANGE         = 0x18,
  EXTINT_LINK_STATUS_CHANGE_EVEN_PORTS = 0x19,
  EXTINT_LINK_STATUS_CHANGE_ODD_PORTS  = 0x1A,
  EXTINT_RX_PACKET_ODD_PORTS           = 0x1B,
};

/* Access Utilities
 * ----------------
 * We use functions to access everything rather than things like
 *   #define SOME_REG *(uint32_t*)0xDEADBEEF
 * to give us flexibility for how register accesses are performed, in case we
 * ever want to run the code in an emulator with register accesses forwarded to
 * the actual device, or so on.
 */

// e.g. GetNVIC(NVIC_TABLE_OFFSET);
static inline uint32_t GetNVIC(uint32_t regno) {
  return *(uint32_t*)regno;
}
static inline void SetNVIC(uint32_t regno, uint32_t v) {
  *(volatile uint32_t*)regno = v;
}

static inline void NVICIntEnable(uint32_t extIntNo) {
  SetNVIC(NVIC_INT_ENABLE + (extIntNo/32)*4, 1U<<(extIntNo % 32));
}
static inline void NVICIntDisable(uint32_t extIntNo) {
  SetNVIC(NVIC_INT_DISABLE + (extIntNo/32)*4, 1U<<(extIntNo % 32));
}
static inline void NVICIntClearPending(uint32_t extIntNo) {
  SetNVIC(NVIC_INT_CLEAR_PENDING + (extIntNo/32)*4, 1U<<(extIntNo % 32));
}

// e.g. GetDevReg(0, REG_STATUS);
static inline uint32_t *GetDevRegAddr(uint8_t func, uint32_t regno) {
  return (uint32_t*)(APE_DEVREG_BASE + (((uint32_t)func)*0x10000) + regno);
}
static inline uint32_t GetDevReg(uint8_t func, uint32_t regno) {
  return *GetDevRegAddr(func, regno);
}
static inline void SetDevReg(uint8_t func, uint32_t regno, uint32_t v) {
  *GetDevRegAddr(func, regno) = v;
}
static inline void OrDevReg(uint8_t func, uint32_t regno, uint32_t v) {
  SetDevReg(func, regno, GetDevReg(func, regno) | v);
}
static inline void MaskDevReg(uint8_t func, uint32_t regno, uint32_t v) {
  SetDevReg(func, regno, GetDevReg(func, regno) & ~v);
}

// e.g. GetSHMAddr(0, REG_APE__RCPU_SEG_SIG).
static inline uint32_t *GetSHMAddr(uint8_t func, uint32_t offset) {
  return (uint32_t*)(APE_SHM_BASE + (((uint32_t)func)*0x1000) + offset - APE_REG(0x4000));
}
static inline uint32_t GetSHM(uint8_t func, uint32_t offset) {
  return *GetSHMAddr(func, offset);
}
static inline void SetSHM(uint8_t func, uint32_t offset, uint32_t v) {
  *GetSHMAddr(func, offset) = v;
}
static inline void OrSHM(uint8_t func, uint32_t offset, uint32_t v) {
  *GetSHMAddr(func, offset) |= v;
}
static inline void MaskSHM(uint8_t func, uint32_t offset, uint32_t bitsToMask) {
  *GetSHMAddr(func, offset) &= ~bitsToMask;
}
static inline void SetSHMBit(uint8_t func, uint32_t offset, uint32_t bit, bool on) {
  SetSHM(func, offset, on ? (GetSHM(func, offset) | bit) : (GetSHM(func, offset) & ~bit));
}

// e.g. GetAPEReg(REG_APE__MODE);
static inline volatile uint32_t *GetAPERegAddr(uint32_t regno) {
  if (regno >= APE_REG(0x8000))
    return (uint32_t*)(APE_PERIPHERAL_BASE + regno - APE_REG(0x8000));
  return (uint32_t*)(APE_APEREG_BASE + regno - APE_REG(0));
}
static inline uint32_t GetAPEReg(uint32_t regno) {
  return *GetAPERegAddr(regno);
}
static inline void SetAPEReg(uint32_t regno, uint32_t v) {
  *GetAPERegAddr(regno) = v;
}
static inline void OrAPEReg(uint32_t regno, uint32_t v) {
  SetAPEReg(regno, GetAPEReg(regno) | v);
}
static inline void MaskAPEReg(uint32_t regno, uint32_t v) {
  SetAPEReg(regno, GetAPEReg(regno) & ~v);
}

#if TODO
static inline bool GetMediaMode(uint8_t func) {
  return !!(GetDevReg(func, REG_SGMII_STATUS) & REG_SGMII_STATUS__MEDIA_SELECTION_MODE);
}
static inline bool IsGPHY(uint8_t func) {
  return !GetMediaMode(func);  
}
static inline bool IsSERDES(uint8_t func) {
  return GetMediaMode(func);
}
static inline uint8_t GetPHYNo(uint8_t func) {
  return (GetMediaMode(func)<<3) || (func+IsGPHY(func));
}

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
#endif

/* ISRs
 * ----
 */
INTERRUPT void ISR_Exception(isr_args *args) {
  ISR_GET_REGS();

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
#endif
}

INTERRUPT void ISR_SVCall(isr_args *args) {
}

#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif

INTERRUPT void ISR_SysTick(isr_args *args) {
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}

INTERRUPT void ISR_LinkStatusChange_EvenPorts(isr_args *args) {
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
#endif
}

INTERRUPT void ISR_LinkStatusChange_OddPorts(isr_args *args) {
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

INTERRUPT void ISR_GenStatusChange(isr_args *args) {
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
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
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
#endif

INTERRUPT void ISR_VoltageSourceChange(isr_args *args) {
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

INTERRUPT void ISR_RXPacket_EvenPorts(isr_args *args) {
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
#endif
}

INTERRUPT void ISR_RXPacket_OddPorts(isr_args *args) {
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
#endif
}

INTERRUPT void ISR_RMUEgress(isr_args *args) {
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

INTERRUPT void ISR_H2B(isr_args *args) {
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

INTERRUPT void ISR_Ext0A(isr_args *args) {
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
#endif

INTERRUPT void ISR_HandleEvent(isr_args *args) {
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
#endif
}

// ----------------------------------------------------------------------------

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
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
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

NO_INLINE void APEStart(void) {
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
#endif
}

/* Image Headers and Entrypoint
 * ----------------------------
 */

noreturn void APEHang(void) {
  for(;;);
}

__attribute__((section(".textstart"), naked)) noreturn void APEEntrypoint(isr_args *args) {
  asm(
    "ldr sp, =" STRINGIFY(STACK_END) "\n"
    "bl APEStart\n"
    "bl APEHang\n"
    );
}

extern int _ScratchTextStart, _ScratchTextEnd, _ScratchTextSize, _ScratchTextOffset, _ScratchTextOffsetFlags;
extern int _TextStart, _TextEnd, _TextSize, _TextOffset, _TextOffsetFlags;
extern int _DataStart, _DataEnd, _DataSize, _DataOffset, _DataOffsetFlags;
extern int _BSSStart, _BSSEnd, _BSSSize, _BSSOffset, _BSSOffsetFlags;
extern int _APEEntrypointM;

const __attribute__((section(".header"))) ape_header gAPE_header = {
  .magic          = "BCM\x1A",
  .unk04          = 0x03070700,                 // Unknown, not read by boot ROM.
  .imageName      = APE_IMAGE_NAME " " STRINGIFY(APE_VER_MAJOR) "." STRINGIFY(APE_VER_MINOR) "." STRINGIFY(APE_VER_PATCH),
  .imageVersion   = (APE_VER_MAJOR<<24) | (APE_VER_MINOR<<16) | (APE_VER_PATCH<<8),

  // Use the entrypoint vector with the thumb flag masked out; it's actually
  // completely inconsequential since the boot ROM will add it back in, but
  // the original images don't set it so we don't either.
  .entrypoint     = (uint32_t)&_APEEntrypointM,

  .unk020         = 0x00,                       // Unknown, not read by boot ROM.
  .headerSize     = sizeof(ape_header)/4,
  .unk022         = 0x04,                       // Unknown, not read by boot ROM.
  .numSections    = 4,
  .headerChecksum = 0xDEADBEEF,                 // Checksums are fixed later in apestamp.

  .sections = {
    [0] = { // "Scratchcode"
      .loadAddr         = (uint32_t)&_ScratchTextStart,
      .offsetFlags      = (uint32_t)&_ScratchTextOffsetFlags,
      .uncompressedSize = (uint32_t)&_ScratchTextSize,
      .compressedSize   = 0,
      .checksum         = 0xDEADBEEF,
    },

    [1] = { // "Maincode"
      .loadAddr         = (uint32_t)&_TextStart,
      .offsetFlags      = (uint32_t)&_TextOffsetFlags,
      .uncompressedSize = (uint32_t)&_TextSize,
      .compressedSize   = 0,
      .checksum         = 0xDEADBEEF,
    },

    [2] = { // "Data"
      .loadAddr         = (uint32_t)&_DataStart,
      .offsetFlags      = (uint32_t)&_DataOffsetFlags,
      .uncompressedSize = (uint32_t)&_DataSize,
      .compressedSize   = 0,
      .checksum         = 0xDEADBEEF,
    },

    [3] = { // "BSS"
      .loadAddr         = (uint32_t)&_BSSStart,
      .offsetFlags      = (uint32_t)&_BSSOffsetFlags,
      .uncompressedSize = (uint32_t)&_BSSSize,
      .compressedSize   = 0,
      .checksum         = 0,
    },
  },
};

__attribute__((section(".isrtable"))) isr_table g_isrTable = {
  .stackEnd   = (void*)STACK_END,
  .reset      = APEEntrypoint,
  .nmi        = ISR_Exception,
  .hardFault  = ISR_Exception,
  .memManage  = ISR_Exception,
  .busFault   = ISR_Exception,
  .usageFault = ISR_Exception,
  .svCall     = ISR_SVCall,
  .debugMon   = ISR_Exception,
  .pendSV     = ISR_Exception,
  .sysTick    = ISR_SysTick,
};

__attribute__((section(".trailingcrc"))) uint32_t g_trailingCRC = 0xDEADBEEF;
