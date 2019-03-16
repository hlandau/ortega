#include "otg.h"
#include <stdint.h>
#include <stdnoreturn.h>

/* Image Build Information {{{1
 * -----------------------
 */
#define APE_IMAGE_NAME "OTG NCSI"
#define APE_VER_MAJOR 1
#define APE_VER_MINOR 3
#define APE_VER_PATCH 7

// Originally these used dates/times in the format "Dec  3 2014" and "15:25:28"
// respectively. We don't use dates/times since this needlessly affects build
// determinism. We use something else meaningful instead. Note that these strings
// must remain exactly 11 and 8 characters long, respectively.
#define BUILD_DATE    "OTG OTG OTG"
#define BUILD_TIME    "HAMSTROK"
static_assert(sizeof(BUILD_DATE) == (11+1), "build date");
static_assert(sizeof(BUILD_TIME) == ( 8+1), "build time");

/* Globals {{{1
 * -------
 */
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif

/* Utilities {{{1
 * ---------
 */
void *memset(void *p, int c, size_t n) {
  uint8_t *p_ = p;
  for (size_t i=0; i<n; ++i)
    *p_++ = c;
  return p;
}

void *memcpy(void *d, const void *s, size_t n) {
  uint32_t *d32 = d;
  const uint32_t *s32 = s;

  for (; n >= 4; n -= 4)
    *d32++ = *s32++;

  uint8_t *d8 = (uint8_t*)d32;
  const uint8_t *s8 = (const uint8_t*)s32;
  for (; n > 0; --n)
    *d8++ = *s8++;

  return d;
}

/* ISRs {{{1
 * ----
 */
extern isr_table g_isrTable;

noreturn void AHang(void) {
  for (;;)
    asm volatile("wfi");
}

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
#endif
}

INTERRUPT void ISR_SysTick(isr_args *args) {
  ISR_GET_REGS();
}

INTERRUPT void ISR_SVCall(isr_args *args) {
  ISR_GET_REGS();
}

INTERRUPT void ISR_RXEvenPorts(isr_args *args) {
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
#endif
}

INTERRUPT void ISR_RXOddPorts(isr_args *args) {
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
#endif
}

INTERRUPT void ISR_RMUEgress(isr_args *args) {
  ISR_GET_REGS();

#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif
}


/* Utilities {{{1
 * ---------
 */
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif

/* TX To Network {{{1
 * -------------
 */
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
#endif

/* RX From Network {{{1
 * ---------------
 */
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
#endif

/* RMU {{{1
 * ---
 */
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
#endif

/* B2H {{{1
 * ===
 */
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

/* H2B {{{1
 * ===
 */
#ifdef PROPRIETARY
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
/*               scrubbed                    */
#endif

/* NCSI {{{1
 * ----
 */
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
#endif

/* Main {{{1
 * ----
 */
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
#endif

/* Init {{{1
 * ----
 */
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
#endif

NO_INLINE void AStart(void) {
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


/* Image Headers and Entrypoint {{{1
 * ----------------------------
 */
__attribute__((section(".textstart"), naked)) noreturn void AEntrypoint(isr_args *args) {
  asm(
    "ldr sp, =" STRINGIFY(STACK_END) "\n"
    "bl AStart\n"
    "bl AHang\n"
    );
}

extern int _ScratchTextStart, _ScratchTextEnd, _ScratchTextSize, _ScratchTextOffset, _ScratchTextOffsetFlags;
extern int _TextStart, _TextEnd, _TextSize, _TextOffset, _TextOffsetFlags;
extern int _DataStart, _DataEnd, _DataSize, _DataOffset, _DataOffsetFlags;
extern int _BSSStart, _BSSEnd, _BSSSize, _BSSOffset, _BSSOffsetFlags;
extern int _AEntrypointM;

const __attribute__((section(".header"))) ape_header gAPE_header = {
  .magic          = "BCM\x1A",
  .unk04          = 0x03070700,                 // Unknown, not read by boot ROM.
  .imageName      = APE_IMAGE_NAME " " STRINGIFY(APE_VER_MAJOR) "." STRINGIFY(APE_VER_MINOR) "." STRINGIFY(APE_VER_PATCH),
  .imageVersion   = (APE_VER_MAJOR<<24) | (APE_VER_MINOR<<16) | (APE_VER_PATCH<<8),

  // Use the entrypoint vector with the thumb flag masked out; it's actually
  // completely inconsequential since the boot ROM will add it back in, but
  // the original images don't set it so we don't either.
  .entrypoint     = (uint32_t)&_AEntrypointM,

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
  .reset      = AEntrypoint,
  .nmi        = ISR_Exception,
  .hardFault  = ISR_Exception,
  .memManage  = ISR_Exception,
  .busFault   = ISR_Exception,
  .usageFault = ISR_Exception,
  .svCall     = ISR_SVCall,
  .debugMon   = ISR_Exception,
  .pendSV     = ISR_Exception,
  .sysTick    = ISR_SysTick,
  .ext = {
    [EXTINT_HANDLE_EVENT]                   = ISR_Exception,
    [EXTINT_03H]                            = ISR_Exception,
    [EXTINT_H2B]                            = ISR_Exception,
    [EXTINT_TX_ERR]                         = ISR_Exception,
    [EXTINT_RX_PACKET_EVEN_PORTS]           = ISR_RXEvenPorts,
    [EXTINT_SMBUS_0]                        = ISR_Exception,
    [EXTINT_SMBUS_1]                        = ISR_Exception,
    [EXTINT_RMU_EGRESS]                     = ISR_RMUEgress,
    [EXTINT_GEN_STATUS_CHANGE]              = ISR_Exception,
    [EXTINT_VOLTAGE_SOURCE_CHANGE]          = ISR_Exception,
    [EXTINT_LINK_STATUS_CHANGE_EVEN_PORTS]  = ISR_Exception,
    [EXTINT_LINK_STATUS_CHANGE_ODD_PORTS]   = ISR_Exception,
    [EXTINT_RX_PACKET_ODD_PORTS]            = ISR_RXOddPorts,
  },
};

__attribute__((section(".trailingcrc"))) uint32_t g_trailingCRC = 0xDEADBEEF;
