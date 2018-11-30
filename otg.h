#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#ifdef OTG_HOST
#  include <stdlib.h>
#  include <unistd.h>
#endif

#define OTG_DEBUG
#define TARGET_MODEL 5719
#define ENABLE_NCSI 0
#define PROPRIETARY 1

/* 0: Support either GPHY (copper) or SERDES (SFP) NICs at runtime.
 * 1: Support GPHY only (hardcoded).
 * 2: Support SERDES only (hardcoded).
 */
#define TARGET_MEDIA 0

/* ---------------------------------------------------------------- */
#if TARGET_MODEL == 5719
#  define TARGET_PORTS 4
#else
#  error Unsupported device
#endif

#ifndef TARGET_PORTS_ENABLED
#  define TARGET_PORTS_ENABLED TARGET_PORTS
#endif

#if TARGET_PORTS_ENABLED < 0 || TARGET_PORTS_ENABLED > TARGET_PORTS
#  error Invalid number of enabled target ports
#endif

#if TARGET_MEDIA != 0 && TARGET_MEDIA != 1 && TARGET_MEDIA != 2
#  error Invalid TARGET_MEDIA setting.
#endif

#if TARGET_MEDIA == 2 && TARGET_MODEL == 5717
#  error 5717 does not support SERDES
#endif

#define PORT_MASK (TARGET_PORTS-1)


/* ---------------------------------------------------------------- */

#define static_assert _Static_assert
#ifndef ARRAYLEN
#  define ARRAYLEN(X) (sizeof(X)/sizeof((X)[0]))
#endif

// check these
#define BIT(Num) (UINT32_C(1)<<(Num))
#define BITS(LowerIncl, UpperIncl) \
  (((UINT32_C(0xFFFFFFFF) >> (LowerIncl)) << (LowerIncl)) << (31-(UpperIncl)) >> (31-(UpperIncl)))

#define REGMEM_BASE 0xC0000000
#define GENCOM_BASE 0xB50

#define V1_45

// v1.39: 0x0127, v1.45: 0x012D, etc.
#define RX_FW_VERSION  0x012D

/* ---------------------------------------------------------------- */

#define REG_POWER_MANAGEMENT_CONTROL_STATUS 0x004C
#define REG_POWER_MANAGEMENT_CONTROL_STATUS__PME_ENABLE 0x00000100

#define REG_MISCELLANEOUS_HOST_CONTROL 0x0068
#define REG_MISCELLANEOUS_HOST_CONTROL__ENABLE_ENDIAN_WORD_SWAP  0x0008
#define REG_MISCELLANEOUS_HOST_CONTROL__ENABLE_ENDIAN_BYTE_SWAP  0x0004

#define REG_PCI_STATE 0x0070
#define REG_PCI_STATE__APE_CONTROL_REGISTER_WRITE_ENABLE  0x00010000
#define REG_PCI_STATE__APE_SHARED_MEMORY_WRITE_ENABLE     0x00020000
#define REG_PCI_STATE__APE_PROGRAM_SPACE_WRITE_ENABLE     0x00040000
#define REG_PCI_STATE__PCI_EXPANSION_ROM_DESIRED          0x00000010

#define REG_MEMORY_BASE 0x007C

#define REG_UNDI_RECEIVE_RETURN_RING_CONSUMER_INDEX      0x0088
#define REG_UNDI_RECEIVE_RETURN_RING_CONSUMER_INDEX_LOW  0x008C

#define REG_LINK_STATUS_CONTROL 0x00BC
#define REG_LINK_STATUS_CONTROL__NEG_LINK_WIDTH__MASK 0x03F00000
#define REG_LINK_STATUS_CONTROL__NEG_LINK_SPEED__MASK 0x000F0000
#define REG_LINK_STATUS_CONTROL__NEG_LINK_SPEED__1 0x00010000
#define REG_LINK_STATUS_CONTROL__NEG_LINK_SPEED__2 0x00020000

// "APE Scratchpad"
//
// This might be an undocumented way to gain access to APE code memory in a
// similar vein to the other "indirect access" register sets in in early device
// register space.
//
// Write the address to 0x00F8, then write the word you want to write to 0x00FC.
// Uses configuration space, will the copy of it in the first BAR also work?
//
// Known use: An APE image (including image headers) is written to "0xD800" (in
// terms of the address written to 0x00F8), then the address 0x10_D800 (|2 for
// some reason) is written to REG_APE__GPIO_MSG. Seems to be the address of
// what was written so this is probably relative to APEMEM 0x10_0000, which is
// roughly where the code lives.
#define REG_UNK_F8  0x00F8
#define REG_UNK_FC  0x00FC

#define REG_EMAC_MODE 0x0400
#define REG_EMAC_MODE__PORT_MODE_MASK 0x000C
#define REG_EMAC_MODE__PORT_MODE_1000 0x0008 /* Gigabit mode */
#define REG_EMAC_MODE__PORT_MODE_100  0x0004 /* 10/100 mode  */
#define REG_EMAC_MODE__PORT_MODE_NONE 0x0000
#define REG_EMAC_MODE__ACPI_POWER_ON_ENABLE 0x00080000
#define REG_EMAC_MODE__MAGIC_PACKET_DETECTION_ENABLE 0x00040000
#define REG_EMAC_MODE__MAC_LOOPBACK_MODE_CONTROL 0x20000000
#define REG_EMAC_MODE__ENABLE_APE_TX_PATH 0x10000000

#define REG_EMAC_STATUS 0x0404
#define REG_EMAC_STATUS__LINK_STATE_CHANGED    0x1000

#define REG_LED_CONTROL 0x040C
#define REG_LED_CONTROL__OVERRIDE_LINK                 0x00000001
#define REG_LED_CONTROL__LED_1000                      0x00000002
#define REG_LED_CONTROL__LED_100                       0x00000004
#define REG_LED_CONTROL__LED_10                        0x00000008
#define REG_LED_CONTROL__OVERRIDE_TRAFFIC              0x00000010
#define REG_LED_CONTROL__LED_TRAFFIC_BLINK             0x00000020
#define REG_LED_CONTROL__LED_TRAFFIC                   0x00000040
#define REG_LED_CONTROL__LED_MODE__MAC                 0x00000000
#define REG_LED_CONTROL__LED_MODE__PHY_MODE_1          0x00000800
#define REG_LED_CONTROL__LED_MODE__PHY_MODE_2          0x00001000
#define REG_LED_CONTROL__LED_MODE__PHY_MODE_1_         0x00001800
#define REG_LED_CONTROL__LED_MODE__MASK                0x00001800
#define REG_LED_CONTROL__LED_MODE__SHIFT               11
#define REG_LED_CONTROL__SHARED_TRAFFIC_LINK_LED_MODE  0x00004000
#define REG_LED_CONTROL__MAC_MODE                      0x00002000

#define REG_EMAC_MAC_ADDRESSES_0_HIGH 0x0410
#define REG_EMAC_MAC_ADDRESSES_0_LOW  0x0414
#define REG_EMAC_MAC_ADDRESSES_1_HIGH 0x0418
#define REG_EMAC_MAC_ADDRESSES_1_LOW  0x041C
#define REG_EMAC_MAC_ADDRESSES_2_HIGH 0x0420
#define REG_EMAC_MAC_ADDRESSES_2_LOW  0x0424
#define REG_EMAC_MAC_ADDRESSES_3_HIGH 0x0428
#define REG_EMAC_MAC_ADDRESSES_3_LOW  0x042C

#define REG_MII_COMMUNICATION 0x044C
#define REG_MII_COMMUNICATION__START_BUSY 0x20000000
#define REG_MII_COMMUNICATION__CMD_READ   0x08000000
#define REG_MII_COMMUNICATION__CMD_WRITE  0x04000000

#define REG_MII_MODE 0x0454
#define REG_MII_MODE__CONSTANT_MDIO_MDC_CLOCK_SPEED           0x00008000

#define REG_RECEIVE_MAC_MODE 0x0468
#define REG_RECEIVE_MAC_MODE__ENABLE 0x0002
#define REG_RECEIVE_MAC_MODE__PROMISCUOUS_MODE 0x0010

// According to the register manual, these set the MAC addresses assigned to
// the APE for RX matching purposes. The diag tools consider PERFECT_MATCH1 to
// be the "APE MAC"...
#define REG_APE_PERFECT_MATCH1_HIGH 0x0540
#define REG_APE_PERFECT_MATCHN_HIGH(N) (0x0540+8*(N))
#define REG_APE_PERFECT_MATCH1_LOW  0x0544
#define REG_APE_PERFECT_MATCHN_LOW(N)  (0x0544+8*(N))
#define REG_APE_PERFECT_MATCH2_HIGH 0x0548
#define REG_APE_PERFECT_MATCH2_LOW  0x054C
#define REG_APE_PERFECT_MATCH3_HIGH 0x0550
#define REG_APE_PERFECT_MATCH3_LOW  0x0554
#define REG_APE_PERFECT_MATCH4_HIGH 0x0558
#define REG_APE_PERFECT_MATCH4_LOW  0x055C

#define REG_SGMII_STATUS 0x05B4
#define REG_SGMII_STATUS__MEDIA_SELECTION_MODE 0x0100
#define REG_SGMII_STATUS__LINK_STATUS 0x0002
#define REG_SGMII_STATUS__SPEED_100 0x0010

#define REG_HTX2B_PROTOCOL_FILTER 0x06D0
#define REG_HTX2B_PROTOCOL_FILTER__ADDR0_ENABLE  0x00000001
#define REG_HTX2B_PROTOCOL_FILTER__ADDR1_ENABLE  0x00000002
#define REG_HTX2B_PROTOCOL_FILTER__ADDR2_ENABLE  0x00000004
#define REG_HTX2B_PROTOCOL_FILTER__ADDR3_ENABLE  0x00000008
#define REG_HTX2B_PROTOCOL_FILTER__ADDR_ENABLE__MASK  0x0000000F

#define REG_HTX2B_GLOBAL_FILTER 0x06D4
#define REG_HTX2B_GLOBAL_FILTER__MULTICAST_FILTER_ENABLE 0x01
#define REG_HTX2B_GLOBAL_FILTER__BROADCAST_FILTER_ENABLE 0x02

#define REG_APE_NETWORK_STATS_IFHCINOCTETS_GOOD          0x0980
#define REG_APE_NETWORK_STATS_IFHCINOCTETS_BAD           0x0984

#define REG_CPMU_CONTROL  0x3600
#define REG_CPMU_CONTROL__LINK_IDLE_POWER_MODE_ENABLE 0x00000200
#define REG_CPMU_CONTROL__LINK_AWARE_POWER_MODE_ENABLE 0x00000400
#define REG_CPMU_CONTROL__LINK_SPEED_POWER_MODE_ENABLE 0x00004000

#define REG_LINK_AWARE_POWER_MODE_CLOCK_POLICY 0x3610
#define REG_LINK_AWARE_POWER_MODE_CLOCK_POLICY__MAC_CLOCK_SWITCH__MASK     0x001F0000
#define REG_LINK_AWARE_POWER_MODE_CLOCK_POLICY__MAC_CLOCK_SWITCH__6_25MHZ  0x00130000

#define REG_D0U_CLOCK_POLICY 0x3614
#define REG_D0U_CLOCK_POLICY__MAC_CLOCK_SWITCH__6_25MHZ   0x00130000

#define REG_APE_SLEEP_STATE_CLOCK_POLICY 0x3620
#define REG_APE_SLEEP_STATE_CLOCK_POLICY__APE_SLEEP_HCLK_DISABLE 0x80000000

#define REG_CLOCK_SPEED_OVERRIDE_POLICY 0x3624
#define REG_CLOCK_SPEED_OVERRIDE_POLICY__MAC_CLOCK_SPEED_OVERRIDE_ENABLE 0x80000000

#define REG_STATUS 0x362C
#define REG_STATUS__FUNC_NUMBER__SHIFT 30
#define REG_STATUS__VMAIN_POWER_STATUS 0x2000
#define REG_STATUS__ETHERNET_LINK_STATUS__MASK  0x00180000
#define REG_STATUS__ETHERNET_LINK_STATUS__1000 0x00000000
#define REG_STATUS__ETHERNET_LINK_STATUS__100  0x00080000
#define REG_STATUS__ETHERNET_LINK_STATUS__10   0x00100000
#define REG_STATUS__ETHERNET_LINK_STATUS__NONE 0x00180000
#define REG_STATUS__WOL_ACPI_DETECTION_ENABLE_PORT_0 0x00008000
#define REG_STATUS__WOL_ACPI_DETECTION_ENABLE_PORT_1 0x00400000
#define REG_STATUS__WOL_MAGIC_PACKET_DETECTION_ENABLE_PORT_0 0x00004000
#define REG_STATUS__WOL_MAGIC_PACKET_DETECTION_ENABLE_PORT_1 0x00200000
#define REG_STATUS__FUNC_ENABLE_0             0x02000000 /* are these active-low? */
#define REG_STATUS__FUNC_ENABLE_ALL           0x20000000 /* guessed */

#define REG_CLOCK_STATUS 0x3630

#define REG_GPHY_CONTROL_STATUS 0x3638
#define REG_GPHY_CONTROL_STATUS__BIAS_IDDQ                       0x00000002
#define REG_GPHY_CONTROL_STATUS__GPHY_IDDQ                       0x00000001
#define REG_GPHY_CONTROL_STATUS__SGMII_PCS_POWER_DOWN            0x00008000
#define REG_GPHY_CONTROL_STATUS__TLP_CLOCK_SOURCE                0x04000000
#define REG_GPHY_CONTROL_STATUS__SWITCHING_REGULATOR_POWER_DOWN  0x08000000
#define REG_GPHY_CONTROL_STATUS__POWER_DOWN                      0x00000010

#define REG_CHIP_ID 0x3658

#define REG_MUTEX_REQUEST 0x365C
#define REG_MUTEX_GRANT   0x3660

#define REG_GPHY_STRAP 0x3664
#define REG_GPHY_STRAP__TXMBUF_ECC_ENABLE      0x00000004
#define REG_GPHY_STRAP__RXMBUF_ECC_ENABLE      0x00000008
#define REG_GPHY_STRAP__RXCPU_SPAD_ECC_ENABLE  0x00000010

#define REG_TOP_LEVEL_MISCELLANEOUS_CONTROL_1 0x367C
#define REG_TOP_LEVEL_MISCELLANEOUS_CONTROL_1__NCSI_CLOCK_OUTPUT_DISABLE  0x00000010

#define REG_EEE_MODE 0x36B0
#define REG_EEE_MODE__USER_LPI_ENABLE             0x00000080
#define REG_EEE_MODE__TX_LPI_ENABLE               0x00000100
#define REG_EEE_MODE__RX_LPI_ENABLE               0x00000200
#define REG_EEE_MODE__LINK_IDLE_DETECTION_ENABLE  0x00000008

#define REG_EEE_LINK_IDLE_CONTROL 0x36BC
#define REG_EEE_LINK_IDLE_CONTROL__DEBUG_UART_IDLE 0x00000004

#define REG_EEE_CONTROL 0x36D0

#define REG_GLOBAL_MUTEX_REQUEST 0x36F0
#define REG_GLOBAL_MUTEX_GRANT   0x36F4

#define REG_MEMORY_ARBITER_MODE 0x4000
#define REG_MEMORY_ARBITER_MODE__ENABLE 0x0002

#define REG_BUFFER_MANAGER_MODE 0x4400
#define REG_BUFFER_MANAGER_MODE__ENABLE 0x00000002
#define REG_BUFFER_MANAGER_MODE__ATTENTION_ENABLE 0x00000004
#define REG_BUFFER_MANAGER_MODE__RESET_RXMBUF_PTR 0x00000020

#define REG_LSO_READ_DMA_MODE   0x4800
#define REG_LSO_READ_DMA_MODE__INBAND_VLAN_TAG_ENABLE              0x20000000
#define REG_LSO_READ_DMA_MODE__HW_IPV6_POST_DMA_PROCESSING_ENABLE  0x10000000

#define REG_HTX2B_PERFECT_MATCHN_HIGH(N)   (0x4880+8*(N))
#define REG_HTX2B_PERFECT_MATCHN_LOW (N)   (0x4884+8*(N))

#define REG_LSO_NONLSO_BD_READ_DMA_CORRUPTION_ENABLE_CONTROL  0x4910
#define REG_LSO_NONLSO_BD_READ_DMA_CORRUPTION_ENABLE_CONTROL__PCI_REQUEST_BURST_LENGTH_FOR_NONLSO_RDMA_ENGINE__4K  0x000C0000
#define REG_LSO_NONLSO_BD_READ_DMA_CORRUPTION_ENABLE_CONTROL__PCI_REQUEST_BURST_LENGTH_FOR_NONLSO_RDMA_ENGINE__MASK 0x000C0000
#define REG_LSO_NONLSO_BD_READ_DMA_CORRUPTION_ENABLE_CONTROL__PCI_REQUEST_BURST_LENGTH_FOR_BD_RDMA_ENGINE__4K 0x00030000
#define REG_LSO_NONLSO_BD_READ_DMA_CORRUPTION_ENABLE_CONTROL__PCI_REQUEST_BURST_LENGTH_FOR_BD_RDMA_ENGINE__MASK 0x00030000

#define REG_RX_RISC_MODE 0x5000
#define REG_RX_RISC_MODE__ENABLE_DATA_CACHE    0x00000020
#define REG_RX_RISC_MODE__ENABLE_WATCHDOG      0x00000080
#define REG_RX_RISC_MODE__HALT                 0x00000400
#define REG_RX_RISC_MODE__SINGLE_STEP          0x00000002

#define REG_RX_RISC_STATUS 0x5004
#define REG_RX_RISC_STATUS__HARDWARE_BREAKPOINT           0x00000001
#define REG_RX_RISC_STATUS__HALT_INSTRUCTION_EXECUTED     0x00000002
#define REG_RX_RISC_STATUS__INVALID_INSTRUCTION           0x00000004
#define REG_RX_RISC_STATUS__PAGE_0_DATA_REFERENCE         0x00000008
#define REG_RX_RISC_STATUS__PAGE_0_INSTRUCTION_REFERENCE  0x00000010
#define REG_RX_RISC_STATUS__INVALID_DATA_ACCESS           0x00000020
#define REG_RX_RISC_STATUS__INVALID_INSTRUCTION_FETCH     0x00000040
#define REG_RX_RISC_STATUS__BAD_MEMORY_ALIGNMENT          0x00000080
#define REG_RX_RISC_STATUS__MEMORY_ADDRESS_TRAP           0x00000100
#define REG_RX_RISC_STATUS__REGISTER_ADDRESS_TRAP         0x00000200
#define REG_RX_RISC_STATUS__HALTED                        0x00000400
#define REG_RX_RISC_STATUS__BIT_11                        0x00000800 /* Undocumented but appears to be a W2C halt condition */
#define REG_RX_RISC_STATUS__BIT_12                        0x00001000 /* Not yet observed */
#define REG_RX_RISC_STATUS__BIT_13                        0x00002000 /* Not yet observed */
#define REG_RX_RISC_STATUS__DATA_ACCESS_STALL             0x00004000
#define REG_RX_RISC_STATUS__INSTRUCTION_FETCH_STALL       0x00008000
#define REG_RX_RISC_STATUS__BLOCKING_READ                 0x80000000
#define REG_RX_RISC_STATUS__CLEARABLE_MASK                (REG_RX_RISC_STATUS__HARDWARE_BREAKPOINT          \
                                                          |REG_RX_RISC_STATUS__HALT_INSTRUCTION_EXECUTED    \
                                                          |REG_RX_RISC_STATUS__INVALID_INSTRUCTION          \
                                                          |REG_RX_RISC_STATUS__PAGE_0_DATA_REFERENCE        \
                                                          |REG_RX_RISC_STATUS__PAGE_0_INSTRUCTION_REFERENCE \
                                                          |REG_RX_RISC_STATUS__INVALID_DATA_ACCESS          \
                                                          |REG_RX_RISC_STATUS__INVALID_INSTRUCTION_FETCH    \
                                                          |REG_RX_RISC_STATUS__BAD_MEMORY_ALIGNMENT         \
                                                          |REG_RX_RISC_STATUS__MEMORY_ADDRESS_TRAP          \
                                                          |REG_RX_RISC_STATUS__REGISTER_ADDRESS_TRAP        \
                                                          |REG_RX_RISC_STATUS__BIT_11                       \
                                                          )

#define REG_RX_RISC_PROGRAM_COUNTER 0x501C

// This undocumented register contains the current word located at the program
// counter address loaded in REG_RX_RISC_PROGRAM_COUNTER.
#define REG_RX_RISC_CUR_INSTRUCTION 0x5020

// These undocumented registers contain the current values in the CPU register
// file. They can be read and written (at least when the CPU is halted, not
// tested otherwise.)
#define REG_RX_RISC_REG_0     0x5200 /* $zero (R0) */
#define REG_RX_RISC_REG_ZERO  0x5200
#define REG_RX_RISC_REG_1     0x5204 /* $at (R1) */
#define REG_RX_RISC_REG_AT    0x5204
#define REG_RX_RISC_REG_2     0x5208 /* $v0 (R2) -- Confirmed */
#define REG_RX_RISC_REG_V0    0x5208
#define REG_RX_RISC_REG_3     0x520C /* $v1 (R3) -- Confirmed */
#define REG_RX_RISC_REG_V1    0x520C
#define REG_RX_RISC_REG_4     0x5210 /* $a0 (R4) -- Confirmed */
#define REG_RX_RISC_REG_A0    0x5210
#define REG_RX_RISC_REG_5     0x5214 /* $a1 (R5) -- Confirmed */
#define REG_RX_RISC_REG_A1    0x5214
#define REG_RX_RISC_REG_6     0x5218 /* $a2 (R6) */
#define REG_RX_RISC_REG_A2    0x5218
#define REG_RX_RISC_REG_7     0x521C /* $a3 (R7) */
#define REG_RX_RISC_REG_A3    0x521C
#define REG_RX_RISC_REG_8     0x5220 /* $t0 (R8) */
#define REG_RX_RISC_REG_T0    0x5220
#define REG_RX_RISC_REG_9     0x5224 /* $t1 (R9) */
#define REG_RX_RISC_REG_T1    0x5224
#define REG_RX_RISC_REG_10    0x5228 /* $t2 (R10) */
#define REG_RX_RISC_REG_T2    0x5228
#define REG_RX_RISC_REG_11    0x522C /* $t3 (R11) */
#define REG_RX_RISC_REG_T3    0x522C
#define REG_RX_RISC_REG_12    0x5230 /* $t4 (R12) */
#define REG_RX_RISC_REG_T4    0x5230
#define REG_RX_RISC_REG_13    0x5234 /* $t5 (R13) */
#define REG_RX_RISC_REG_T5    0x5234
#define REG_RX_RISC_REG_14    0x5238 /* $t6 (R14) */
#define REG_RX_RISC_REG_T6    0x5238
#define REG_RX_RISC_REG_15    0x523C /* $t7 (R15) */
#define REG_RX_RISC_REG_T7    0x523C
#define REG_RX_RISC_REG_16    0x5240 /* $s0 (R16) */
#define REG_RX_RISC_REG_S0    0x5240
#define REG_RX_RISC_REG_17    0x5244 /* $s1 (R17) */
#define REG_RX_RISC_REG_S1    0x5244
#define REG_RX_RISC_REG_18    0x5248 /* $s2 (R18) */
#define REG_RX_RISC_REG_S2    0x5248
#define REG_RX_RISC_REG_19    0x524C /* $s3 (R19) */
#define REG_RX_RISC_REG_S3    0x524C
#define REG_RX_RISC_REG_20    0x5250 /* $s4 (R20) */
#define REG_RX_RISC_REG_S4    0x5250
#define REG_RX_RISC_REG_21    0x5254 /* $s5 (R21) */
#define REG_RX_RISC_REG_S5    0x5254
#define REG_RX_RISC_REG_22    0x5258 /* $s6 (R22) */
#define REG_RX_RISC_REG_S6    0x5258
#define REG_RX_RISC_REG_23    0x525C /* $s7 (R23) */
#define REG_RX_RISC_REG_S7    0x525C
#define REG_RX_RISC_REG_24    0x5260 /* $t8 (R24) */
#define REG_RX_RISC_REG_T8    0x5260
#define REG_RX_RISC_REG_25    0x5264 /* $t9 (R25) */
#define REG_RX_RISC_REG_T9    0x5264
#define REG_RX_RISC_REG_26    0x5268 /* $k0 (R26) */
#define REG_RX_RISC_REG_K0    0x5268
#define REG_RX_RISC_REG_27    0x526C /* $k1 (R27) */
#define REG_RX_RISC_REG_K1    0x526C
#define REG_RX_RISC_REG_28    0x5270 /* $gp (R28) */
#define REG_RX_RISC_REG_GP    0x5270
#define REG_RX_RISC_REG_29    0x5274 /* $sp (R29) */
#define REG_RX_RISC_REG_SP    0x5274
#define REG_RX_RISC_REG_30    0x5278 /* $fp (R30) */
#define REG_RX_RISC_REG_FP    0x5278
#define REG_RX_RISC_REG_31    0x527C /* $ra (R31) -- Confirmed */
#define REG_RX_RISC_REG_RA    0x527C

#define REG_PCI_SERIAL_NUMBER_LOW      0x6504 /* Guessed */
#define REG_PCI_SERIAL_NUMBER_HIGH     0x6508 /* Guessed */

#define REG_PCI_POWER_CONSUMPTION_INFO 0x6410 /* Guessed */
#define REG_PCI_POWER_DISSIPATED_INFO  0x6414 /* Guessed */
#define REG_PCI_VPD_REQ                0x642C /* Guessed */
#define REG_PCI_VPD_RES                0x6430 /* Guessed */

#define REG_PCI_VENDOR_DEVICE_ID 0x6434
  // PCI Vendor/Device ID.
  // Upper 16 bits: Vendor ID. Lower 16 bits: Device ID.
  // Set by S1MegaUltraInit from IDs specified in NVM.
#define DEFAULT_VENDOR_DEVICE_ID 0x14E41657 /* 5719 specific */

#define REG_PCI_SUBSYSTEM_ID     0x6438
  // PCI Subsystem/Subsystem Vendor ID.
  // Upper 16 bits: Subsystem ID. Lower 16 bits: Subsystem Vendor ID.
  // Set by S1MegaUltraInit from IDs specified in NVM.

#define REG_PCI_CLASS_CODE_REVISION    0x643C /* Guessed */

#define REG_PCI_POWER_BUDGET_0         0x6510 /* Guessed */
#define REG_PCI_POWER_BUDGET__COUNT    8

#define REG_GRC_MODE_CONTROL 0x6800
#define REG_GRC_MODE_CONTROL__PCIE_TL_DL_PL_MAPPING_1 0x00400000
#define REG_GRC_MODE_CONTROL__PCIE_TL_DL_PL_MAPPING_2 0x20000000
#define REG_GRC_MODE_CONTROL__PCIE_TL_DL_PL_MAPPING_3 0x80000000
#define REG_GRC_MODE_CONTROL__PCIE_TL_DL_PL__MASK     0xA0400000
#define REG_GRC_MODE_CONTROL__HOST_STACK_UP           0x00010000
#define REG_GRC_MODE_CONTROL__TIME_SYNC_MODE_ENABLE   0x00080000
#define REG_GRC_MODE_CONTROL__NVRAM_WRITE_ENABLE      0x00200000
#define REG_GRC_MODE_CONTROL__B2HRX_BYTE_SWAP         0x00000080
#define REG_GRC_MODE_CONTROL__B2HRX_WORD_SWAP         0x00000040

#define REG_MISCELLANEOUS_CONFIG  0x6804
#define REG_MISCELLANEOUS_CONFIG__PME_ENABLE    0x80000

#define REG_MISCELLANEOUS_LOCAL_CONTROL 0x6808
#define REG_MISCELLANEOUS_LOCAL_CONTROL__AUTO_SEEPROM_ACCESS  0x01000000
#define REG_MISCELLANEOUS_LOCAL_CONTROL__GPIO_0_OUTPUT_ENABLE 0x00000800
#define REG_MISCELLANEOUS_LOCAL_CONTROL__GPIO_1_OUTPUT_ENABLE 0x00001000
#define REG_MISCELLANEOUS_LOCAL_CONTROL__GPIO_2_OUTPUT_ENABLE 0x00002000
#define REG_MISCELLANEOUS_LOCAL_CONTROL__GPIO_0_OUTPUT        0x00004000
#define REG_MISCELLANEOUS_LOCAL_CONTROL__GPIO_1_OUTPUT        0x00008000
#define REG_MISCELLANEOUS_LOCAL_CONTROL__GPIO_2_OUTPUT        0x00010000

#define REG_TIMER 0x680C

#define REG_RX_CPU_EVENT 0x6810
#define REG_RX_CPU_EVENT__VPD_ATTENTION 0x40000000 /* SW Event 12/VPD Attention */

/* The register manual only mentions this in the changelog; it was
 * removed from the manual in a previous revision. :|
 */
#define REG_MDI_CONTROL 0x6844

#define REG_FAST_BOOT_PROGRAM_COUNTER 0x6894

#define REG_EXPANSION_ROM_ADDRESS 0x68EC

#define REG_EAV_REF_CLOCK_CONTROL 0x6908
#define REG_EAV_REF_CLOCK_CONTROL__TIMESYNC_GPIO_MAPPING__MASK 0x00030000
#define REG_EAV_REF_CLOCK_CONTROL__TIMESYNC_GPIO_MAPPING__SHIFT 16
#define REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_0_MAPPING__MASK    0x001C0000
#define REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_0_MAPPING__SHIFT   18
#define REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_1_MAPPING__MASK    0x00E00000
#define REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_1_MAPPING__SHIFT   21
#define REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_2_MAPPING__MASK    0x07000000
#define REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_2_MAPPING__SHIFT   24
#define REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_3_MAPPING__MASK    0x38000000
#define REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_3_MAPPING__SHIFT   27

#define REG_NVM_COMMAND 0x7000
#define REG_NVM_COMMAND__DONE   0x0008
#define REG_NVM_COMMAND__DOIT   0x0010
#define REG_NVM_COMMAND__WR     0x0020
#define REG_NVM_COMMAND__FIRST  0x0080
#define REG_NVM_COMMAND__LAST   0x0100

#define REG_NVM_WRITE 0x7008
#define REG_NVM_ADDRESS 0x700C
#define REG_NVM_READ 0x7010

#define REG_NVM_CONFIG_1 0x7014
#define REG_NVM_CONFIG_1__PROTECT_MODE 0x01000000
#define REG_NVM_CONFIG_1__FLASH_SIZE   0x02000000
#define REG_NVM_CONFIG_1__FLASH_MODE   0x00000001
#define REG_NVM_CONFIG_1__BUFFER_MODE  0x00000002
#define REG_NVM_CONFIG_1__PAGE_SIZE__SHIFT  28
#define REG_NVM_CONFIG_1__PAGE_SIZE__MASK   0x70000000
#define REG_NVM_CONFIG_1__PAGE_SIZE_264     0x50000000
#define REG_NVM_CONFIG_1__SPI_CLK_DIV__MASK   0x00000780
#define REG_NVM_CONFIG_1__SPI_CLK_DIV__SHIFT  7

#define REG_SOFTWARE_ARBITRATION 0x7020
#define REG_SOFTWARE_ARBITRATION__ARB_WON0 0x0100
#define REG_SOFTWARE_ARBITRATION__REQ_SET0 0x0001
#define REG_SOFTWARE_ARBITRATION__REQ_CLR0 0x0010

#define REG_SOFTWARE_ARBITRATION__ARB_WON1 0x0200
#define REG_SOFTWARE_ARBITRATION__REQ_SET1 0x0002
#define REG_SOFTWARE_ARBITRATION__REQ_CLR1 0x0020

#define REG_SOFTWARE_ARBITRATION__ARB_WON2 0x0400
#define REG_SOFTWARE_ARBITRATION__REQ_SET2 0x0004
#define REG_SOFTWARE_ARBITRATION__REQ_CLR2 0x0040

#define REG_SOFTWARE_ARBITRATION__ARB_WON3 0x0800
#define REG_SOFTWARE_ARBITRATION__REQ_SET3 0x0008
#define REG_SOFTWARE_ARBITRATION__REQ_CLR3 0x0080

#define REG_NVM_ACCESS 0x7024
#define REG_NVM_ACCESS__NVM_ACCESS_ENABLE 0x0001
#define REG_NVM_ACCESS__NVM_ACCESS_WRITE_ENABLE 0x0002

#ifdef OTG_HOST
#  define REG_SOFTWARE_ARBITRATION__ARB_WONX REG_SOFTWARE_ARBITRATION__ARB_WON1
#  define REG_SOFTWARE_ARBITRATION__REQ_SETX REG_SOFTWARE_ARBITRATION__REQ_SET1
#  define REG_SOFTWARE_ARBITRATION__REQ_CLRX REG_SOFTWARE_ARBITRATION__REQ_CLR1
#else
#  define REG_SOFTWARE_ARBITRATION__ARB_WONX REG_SOFTWARE_ARBITRATION__ARB_WON0
#  define REG_SOFTWARE_ARBITRATION__REQ_SETX REG_SOFTWARE_ARBITRATION__REQ_SET0
#  define REG_SOFTWARE_ARBITRATION__REQ_CLRX REG_SOFTWARE_ARBITRATION__REQ_CLR0
#endif


/* UNKNOWN REGISTERS
 * -----------------
 */

/*
          0x6400  ]  -- DMA Mode
 UNKNOWN: 0x6408  ]  (0x6400-0x67FF) "PCIe Core Private Registers Access to Configuration Space"
*UNKNOWN: 0x6410  ]  -- NVM 0x0C0. PCIe Power Consumption data.
*UNKNOWN: 0x6414  ]  -- NVM 0x0BC. PCIe Power Dissipated data.
 VPD?     0x642C  ]  -- random guess: PCI VPD Request. could control PCIe VPD capability register, only used in S2 VPD code
 UNKNOWN: 0x6430  ]  -- PCI VPD Response
*         0x6434  ]  -- guessed: PCI Vendor Device ID
*         0x6438  ]  -- guessed: PCI Subsystem/Subsystem Vendor ID
*CL/REV?  0x643C  ]  -- this is a completely random guess, but could be Class Code+Revision ID?
 UNKNOWN: 0x64C0  ]     Chip Revision information is set into 8 bits of it, and other 24 are left
 UNKNOWN: 0x64C8  ]     unchanged.
 UNKNOWN: 0x64DC  ]
 UNKNOWN: 0x6504  ]  -- Device Serial Number Low32
 UNKNOWN: 0x6508  ]  -- Device Serial Number High32
*UNKNOWN:/0x6510]-]-] PCIe Power Budget Values. //// These are set as an array from NVM 0x0E0-0x0F8.
*UNKNOWN:|0x6514] ] ] Only nonzero entries are loaded, and each entry
*UNKNOWN:|0x6518] ] ] can therefore occupy an arbitrary position...
*UNKNOWN:|0x651C] ] ] kinda weird.
*UNKNOWN:|0x6520] ] ]
*UNKNOWN:|0x6524] ] ]    Documentation for REG_GRC_MODE_CONTROL states:
*UNKNOWN:|0x6528] ] ]      "[...] remap PCIe core TL/DL/PL register to GRC space from
*UNKNOWN:\0x652C]-]-]       0x6400 to 0x67FF."
 UNKNOWN: 0x6530  ]
 UNKNOWN: 0x6550  ]  -- part22, GEN_CFG_1E4 bit 0x0040_0000 (LOM Design) sets LSB of this
 UNKNOWN: 0x65F4  ]

 UNKNOWN; 0x6838     Used by PXE agent. tg3: seems to relate to legacy SEEPROM access

 UNKNOWN: 0x68F0  ]  "GRC - Misc Host Control"

 UNKNOWN: 0x7C04  ]  (0x7C00-0x7FFF) "TL-DL-PL Port - PCIe Core Private Register Access to TL, DL & PL"
                     tg3: PCIE_TRANSACTION_CFG
                       PCIE_TRANS_CFG_1SHOT_MSI  0x2000_0000
                       PCIE_TRANS_CFG_LOM        0x0000_0020

 Not touched but mentioned in tg3 and interesting:
   0x7C00  TG3_PCIE_TLDLPL_PORT
   0x7D28  PCIE_PWR_MGMT_THRESH
   0x7D54  TG3_PCIE_LNKCTL
   0x7E2C  TG3_PCIE_PHY_TSTCTL
   0x7E70  TG3_PCIE_EIDLE_DELAY
*/

/* ---------------------------------------------------------------- */
#define MII_REG_CONTROL   0x00
#define MII_REG_CONTROL__AUTO_NEGOTIATION_RESTART 0x0200
#define MII_REG_CONTROL__ISOLATE 0x0400
#define MII_REG_CONTROL__POWER_DOWN 0x0800
#define MII_REG_CONTROL__AUTO_NEGOTIATION_ENABLE 0x1000
#define MII_REG_CONTROL__RESET 0x8000

#define MII_REG_STATUS    0x01
#define MII_REG_STATUS__AUTONEGOTIATION_COMPLETE 0x20
#define MII_REG_PHY_ID_HIGH  0x02
#define MII_REG_PHY_ID_LOW   0x03

#define MII_REG_AUTO_NEGOTIATION_ADVERTISEMENT 0x04
#define MII_REG_AUTO_NEGOTIATION_ADVERTISEMENT__PROTOCOL_SELECT__IEEE_802_3  0x0001
#define MII_REG_AUTO_NEGOTIATION_ADVERTISEMENT__10BASE_T_HALF_DUPLEX 0x0020
#define MII_REG_AUTO_NEGOTIATION_ADVERTISEMENT__10BASE_T_FULL_DUPLEX 0x0040
#define MII_REG_AUTO_NEGOTIATION_ADVERTISEMENT__100BASE_TX_HALF_DUPLEX 0x0080
#define MII_REG_AUTO_NEGOTIATION_ADVERTISEMENT__100BASE_TX_FULL_DUPLEX 0x0100
#define MII_REG_AUTO_NEGOTIATION_ADVERTISEMENT__100BASE_T4 0x0200

#define MII_REG_1000BASE_T_CONTROL 0x09 /* GPHY */
#define MII_REG_1000BASE_T_CONTROL__ADVERTISE_HALF_DUPLEX 0x0100
#define MII_REG_1000BASE_T_CONTROL__ADVERTISE_FULL_DUPLEX 0x0200

#define MII_REG_BCM_SERDES_BLOCKADDRESS 0x1F /* SERDES */

#define MII_REG_SERDES_BLOCK3_ANALOG_TX_1 0x10
#define MII_REG_SERDES_BLOCK3_ANALOG_RX_1 0x13
#define MII_REG_SERDES_BLOCK3_ANALOG_TX_1__IDDQ 0x0001
#define MII_REG_SERDES_BLOCK3_ANALOG_RX_1__IDDQ 0x0001

/* --------------------------------------------------------------------- */
#define DRIVER_READY_MAGIC    0x4B657654 /* 'KevT' */
#define BOOTCODE_READY_MAGIC  0xB49A89AB /* ~'KevT' */
#define BIOS_DISABLED_MAGIC   0xDEADDEAD
#define HEADER_MAGIC          0x669955AA

#define PCI_VENDOR_ID     0x14E4
#define PCI_DEVICE_ID     0x1657
#define PCI_SUBSYSTEM_ID  0x1904
// Talos: 0x1981

#define APE_OFFSET 0x10000
#define APE_REG(X) (APE_OFFSET+(X))

// End of range here is a randomly chosen value, there's nothing nearby so it
// doesn't matter.
#define ROM_START 0x40000000
#define ROM_END   0x40080000

#ifdef __ppc64__
#  define MMIO_BARRIER_PRE()  do { asm volatile ("sync 0\neieio\n" ::: "memory"); } while(0)
#  define MMIO_BARRIER_POST() MMIO_BARRIER_PRE()
#else
#  define MMIO_BARRIER_PRE()  do {} while (0)
#  define MMIO_BARRIER_POST() MMIO_BARRIER_PRE()
#endif

static inline uint32_t Load32(const void *p) {
  MMIO_BARRIER_PRE();
  uint32_t v = *(volatile uint32_t*)p;
  MMIO_BARRIER_POST();
  return v;
}

static inline uint32_t Load32Immutable(const void *p) {
  MMIO_BARRIER_PRE();
  uint32_t v = *(uint32_t*)p;
  MMIO_BARRIER_POST();
  return v;
}

static inline void Store32(void *p, uint32_t value) {
  MMIO_BARRIER_PRE();
  *(volatile uint32_t*)p = value;
  MMIO_BARRIER_POST();
}
static inline void Store16(void *p, uint16_t value) {
  MMIO_BARRIER_PRE();
  *(volatile uint16_t*)p = value;
  MMIO_BARRIER_POST();
}

#ifndef OTG_APE
static inline uint32_t RegNoToRXAddress(uint32_t regno) {
  return REGMEM_BASE+regno;
}
static inline uint32_t GencomToRXAddress(uint32_t gencom) {
  return GENCOM_BASE+gencom;
}

static inline uint32_t GetReg(uint32_t regno);
static inline void SetReg(uint32_t regno, uint32_t value);

#ifdef OTG_HOST
static inline void *GetBAR12Base(void);
static inline void *GetBAR34Base(void);

// Getting access to *arbitrary* RX CPU memory addresses from the host is
// fiddlier than it should be. The following functions implement a variety of
// strategies which are selected according to which are believed to work for
// each memory range. No technique works for all ranges, because some regions
// are execute-only (meaning that even RX CPU loads from those addresses don't
// work). Other regions are RW-only (meaning that the RX CPU can load/store to
// those regions, but instruction fetch from them won't work).

// Get word that's within the first 32k of the device registers, and thus is
// directly accessible via BAR1/2 without messing around with the window. regno
// is relative to BAR1/2 (i.e. 0x00=Device/Vendor ID).
static inline uint32_t GetRXWordLow(uint32_t regno) {
  return Load32((uint8_t*)GetBAR12Base() + regno);
}
static inline uint32_t GetRXWordLowImmutable(uint32_t regno) {
  return Load32Immutable((uint8_t*)GetBAR12Base() + regno);
}
static inline void SetRXWordLow(uint32_t regno, uint32_t value) {
  Store32((uint8_t*)GetBAR12Base() + regno, value);
}
static inline void SetRXWord16Low(uint32_t regno, uint16_t value) {
  Store16((uint8_t*)GetBAR12Base() + regno, value);
}

// Get word that's within the second 32k of BAR1/2 when the window base is set
// to zero, which we assume to generally be the case. Note that we have to XOR
// bit 2 here, because the device swaps the lower and upper 32-bit words of
// each 64 bits by default for accesses to memory (but NOT device registers in
// the first 32k of BAR1/2). This is when REG_MISCELLANEOUS_HOST_CONTROL's
// ENABLE_ENDIAN_WORD_SWAP=0 and ENABLE_ENDIAN_BYTE_SWAP=0, which is the
// default and what the tg3 driver uses, so we assume that these fields are set
// to those values.
static inline uint32_t GetRXWordWindow(uint32_t regno) {
  return Load32((uint8_t*)GetBAR12Base() + (32*1024) + (regno ^ 4));
}
static inline uint32_t GetRXWordWindowImmutable(uint32_t regno) {
  return Load32Immutable((uint8_t*)GetBAR12Base() + (32*1024) + (regno ^ 4));
}
static inline void SetRXWordWindow(uint32_t regno, uint32_t value) {
  Store32((uint8_t*)GetBAR12Base() + (32*1024) + (regno ^ 4), value);
}
static inline void SetRXWord16Window(uint32_t regno, uint16_t value) {
  Store16((uint8_t*)GetBAR12Base() + (32*1024) + (regno ^ 4), value);
}

// Get APE (or higher) word. regno is relative to 0xC001_0000. If we are on the
// host we have to use BAR3/4 to access this space.
static inline uint32_t GetRXWordHigh(uint32_t regno) {
  return Load32((uint8_t*)GetBAR34Base() + regno);
}
static inline uint32_t GetRXWordHighImmutable(uint32_t regno) {
  return Load32Immutable((uint8_t*)GetBAR34Base() + regno);
}
static inline void SetRXWordHigh(uint32_t regno, uint32_t value) {
  Store32((uint8_t*)GetBAR34Base() + regno, value);
}
static inline void SetRXWord16High(uint32_t regno, uint16_t value) {
  Store16((uint8_t*)GetBAR34Base() + regno, value);
}

// Used to access an area of the RX CPU's memory that we don't have a better
// way to access - we can use the above methods for certain ranges/registers
// but in the worst case we have to fall back to this for some areas.
//
// Originally tried to use the memory window register to access the address via
// the window which sits in the upper 32k of BAR1/2, but it doesn't seem to
// work right, at least not for all addresses. So just use the ultimate
// solution: halt the CPU, set the instruction pointer to the address, and read
// out the value which gets loaded into the undocumented "current instruction
// word" register. Then restore the previous IP and unhalt the CPU.
static inline uint32_t GetRXWordViaIP(uint32_t rxAddr) {
  uint32_t oldMode = GetReg(REG_RX_RISC_MODE);
  SetReg(REG_RX_RISC_MODE, oldMode|REG_RX_RISC_MODE__HALT);
  uint32_t oldIP = GetReg(REG_RX_RISC_PROGRAM_COUNTER);

  SetReg(REG_RX_RISC_PROGRAM_COUNTER, rxAddr);
  uint32_t v = GetReg(REG_RX_RISC_CUR_INSTRUCTION);

  SetReg(REG_RX_RISC_PROGRAM_COUNTER, oldIP);
  SetReg(REG_RX_RISC_MODE, oldMode);

  return v;
}
static inline uint32_t GetRXWordViaIPImmutable(uint32_t rxAddr) {
  return GetRXWordViaIP(rxAddr);
}

static inline uint32_t GetRXWordViaWindow(uint32_t rxAddr) {
  uint32_t oldBase = GetReg(REG_MEMORY_BASE);
  //fprintf(stderr, "0x%08X, oldBase=0x%08X, new=0x%08X\n", rxAddr, oldBase, rxAddr & 0xFFFF8000);
  SetReg(REG_MEMORY_BASE, rxAddr & 0xFFFF8000); // Select 32k block
  //fprintf(stderr, "windowSel=0x%08X\n", GetReg(REG_MEMORY_BASE));
  uint32_t v = Load32((uint8_t*)GetBAR12Base() + (32*1024) + (rxAddr & 0x7FFF));
  SetReg(REG_MEMORY_BASE, oldBase);
  return v;
}

static inline void SetRXWordViaWindow(uint32_t rxAddr, uint32_t value) {
  uint32_t oldBase = GetReg(REG_MEMORY_BASE);
  //fprintf(stderr, "0x%08X, oldBase=0x%08X, new=0x%08X\n", rxAddr, oldBase, rxAddr & 0xFFFF8000);
  SetReg(REG_MEMORY_BASE, rxAddr & 0xFFFF8000); // Select 32k block
  //fprintf(stderr, "windowSel=0x%08X\n", GetReg(REG_MEMORY_BASE));
  Store32((uint8_t*)GetBAR12Base() + (32*1024) + (rxAddr & 0x7FFF), value);
  SetReg(REG_MEMORY_BASE, oldBase);
}

static inline uint32_t GetRXWordViaForcedLoad(uint32_t rxAddr) {
  // Halt.
  uint32_t oldMode = GetReg(REG_RX_RISC_MODE);
  SetReg(REG_RX_RISC_MODE, oldMode|REG_RX_RISC_MODE__HALT);

  // Save old state that we will clobber so we can restore it afterwards.
  uint32_t oldIP = GetReg(REG_RX_RISC_PROGRAM_COUNTER);
  uint32_t oldS6 = GetReg(REG_RX_RISC_REG_S6);
  uint32_t oldT7 = GetReg(REG_RX_RISC_REG_T7);

  // Check that the instructions we are expecting to use are correct. This will
  // break if the ROM is different.
  SetReg(REG_RX_RISC_PROGRAM_COUNTER, 0x40000088);
  uint32_t iw = GetReg(REG_RX_RISC_CUR_INSTRUCTION);
  if (iw != 0x8ECF0020) { // lw $t7, 0x20($s6)
    fprintf(stderr, "cannot get RX word via forced load because the device has an unknown ROM (got 0x%08X)\n", iw);
    return 0;
  }

  SetReg(REG_RX_RISC_REG_S6, (rxAddr-0x20));
  SetReg(REG_RX_RISC_MODE, oldMode|REG_RX_RISC_MODE__HALT|REG_RX_RISC_MODE__SINGLE_STEP);

  // Don't remove this, it creates a small delay which seems to sometimes be
  // necessary.
  if (GetReg(REG_RX_RISC_PROGRAM_COUNTER) != 0x4000008C)
    fprintf(stderr, "bad1\n");

  // Get the result.
  uint32_t v = GetReg(REG_RX_RISC_REG_T7);

  // Restore.
  SetReg(REG_RX_RISC_REG_T7, oldT7);
  SetReg(REG_RX_RISC_REG_S6, oldS6);
  SetReg(REG_RX_RISC_PROGRAM_COUNTER, oldIP);
  SetReg(REG_RX_RISC_MODE, oldMode);
  return v;
}

static inline uint32_t GetRXWordViaForcedLoadImmutable(uint32_t rxAddr) {
  return GetRXWordViaForcedLoad(rxAddr);
}

static inline void SetRXWordViaForcedStore(uint32_t rxAddr, uint32_t value) {
  // Halt.
  uint32_t oldMode = GetReg(REG_RX_RISC_MODE);
  SetReg(REG_RX_RISC_MODE, oldMode|REG_RX_RISC_MODE__HALT);

  // Save old state that we will clobber so we can restore it afterwards.
  uint32_t oldIP = GetReg(REG_RX_RISC_PROGRAM_COUNTER);
  uint32_t oldT6 = GetReg(REG_RX_RISC_REG_T6);
  uint32_t oldT7 = GetReg(REG_RX_RISC_REG_T7);

  // Check that the instructions we are expecting to use are correct. This will
  // break if the ROM is different.
  SetReg(REG_RX_RISC_PROGRAM_COUNTER, 0x40000038);
  uint32_t iw = GetReg(REG_RX_RISC_CUR_INSTRUCTION);
  if (iw != 0xADCF0000) { // sw $t7, 0($t6)
    fprintf(stderr, "cannot set RX word via forced store because the device has an unknown ROM (got 0x%08X)\n", iw);
    return;
  }

  SetReg(REG_RX_RISC_REG_T6, rxAddr);
  SetReg(REG_RX_RISC_REG_T7, value);
  SetReg(REG_RX_RISC_MODE, oldMode|REG_RX_RISC_MODE__HALT|REG_RX_RISC_MODE__SINGLE_STEP);

  // Don't remove this, it creates a small delay which seems to sometimes be
  // necessary.
  uint32_t pc = GetReg(REG_RX_RISC_PROGRAM_COUNTER);
  if (pc != 0x4000003C)
    fprintf(stderr, "  bad2 0x%08x\n", pc);

  // Restore.
  SetReg(REG_RX_RISC_REG_T7, oldT7);
  SetReg(REG_RX_RISC_REG_T6, oldT6);
  SetReg(REG_RX_RISC_PROGRAM_COUNTER, oldIP);
  SetReg(REG_RX_RISC_MODE, oldMode);
}

static inline void SetRXWord16ViaForcedStore(uint32_t rxAddr, uint16_t value) {
  // eh
  uint32_t v = GetRXWordViaForcedLoad(rxAddr);
  SetRXWordViaForcedStore(rxAddr, (v & 0x0000FFFF) | (((uint32_t)value) << 16));
}

static inline void SetRXWordMid(uint32_t rxAddr, uint32_t value) {
  fprintf(stderr, "setting midrange RX CPU words is currently not supported\n");
#if 0
  uint32_t oldBase = GetReg(REG_MEMORY_BASE);
  SetReg(REG_MEMORY_BASE, rxAddr & 0xFFFF8000); // Select 32k block
  Store32((uint8_t*)GetBAR12Base() + (32*1024) + (rxAddr & 0x7FFF), value);
  SetReg(REG_MEMORY_BASE, oldBase);
#endif
}
static inline void SetRXWord16Mid(uint32_t rxAddr, uint16_t value) {
  fprintf(stderr, "setting midrange RX CPU words is currently not supported\n");
#if 0
  uint32_t oldBase = GetReg(REG_MEMORY_BASE);
  SetReg(REG_MEMORY_BASE, rxAddr & 0xFFFF8000); // Select 32k block
  Store16((uint8_t*)GetBAR12Base() + (32*1024) + (rxAddr & 0x7FFF), value);
  SetReg(REG_MEMORY_BASE, oldBase);
#endif
}
#endif

static inline uint32_t GetRXWord(uint32_t rxAddr) {
#ifdef OTG_HOST
  if (rxAddr >= REGMEM_BASE && rxAddr < 0xC0008000)
    // Access a device register.
    return GetRXWordLow(rxAddr - REGMEM_BASE);
  else if (rxAddr < 0x8000)
    // First 32k of memory - we can use the window, which we currently assume is set to 0.
    return GetRXWordWindow(rxAddr);
  else if (rxAddr >= (REGMEM_BASE + APE_OFFSET) && rxAddr < 0xC0020000)
    // APE, etc.
    return GetRXWordHigh(rxAddr - (REGMEM_BASE + APE_OFFSET));
  else if (rxAddr >= ROM_START && rxAddr < ROM_END)
    // This is an execute-only range so we can only load it via the instruction
    // fetch hardware.
    return GetRXWordViaIP(rxAddr);
  else
    // RX CPU SRAM.
    return GetRXWordViaForcedLoad(rxAddr);
#else
  return Load32((void*)rxAddr);
#endif
}

static inline uint32_t GetRXWordImmutable(uint32_t rxAddr) {
#ifdef OTG_HOST
  if (rxAddr >= 0xC0000000 && rxAddr < 0xC0008000)
    // Access a device register.
    return GetRXWordLowImmutable(rxAddr - 0xC0000000);
  else if (rxAddr < 0x8000)
    // First 32k of memory - we can use the window, which we currently assume is set to 0.
    return GetRXWordWindowImmutable(rxAddr);
  else if (rxAddr > (REGMEM_BASE + APE_OFFSET) && rxAddr < 0xC0002000)
    // APE, etc.
    return GetRXWordHighImmutable(rxAddr - (REGMEM_BASE + APE_OFFSET));
  else if (rxAddr >= ROM_START && rxAddr < ROM_END)
    // This is an execute-only range so we can only load it via the instruction
    // fetch hardware.
    return GetRXWordViaIPImmutable(rxAddr);
  else
    // RX CPU SRAM.
    return GetRXWordViaForcedLoadImmutable(rxAddr);
#else
  return Load32Immutable((void*)rxAddr);
#endif
}

static inline void SetRXWord(uint32_t rxAddr, uint32_t value) {
#ifdef OTG_HOST
  if (rxAddr >= REGMEM_BASE && rxAddr < 0xC0008000)
    // Access a device register.
    return SetRXWordLow(rxAddr - REGMEM_BASE, value);
  else if (rxAddr < 0x8000)
    // First 32k of memory - we can use the window, which we currently assume is set to 0.
    return SetRXWordWindow(rxAddr, value);
  else if (rxAddr >= (REGMEM_BASE + APE_OFFSET) && rxAddr < 0xC0002000)
    // APE, etc.
    return SetRXWordHigh(rxAddr - (REGMEM_BASE + APE_OFFSET), value);
  else
    // RX CPU SRAM.
    return SetRXWordViaForcedStore(rxAddr, value);
#else
  Store32((void*)rxAddr, value);
#endif
}

static inline void SetRXWord16(uint32_t rxAddr, uint16_t value) {
#ifdef OTG_HOST
  if (rxAddr >= REGMEM_BASE && rxAddr < 0xC0008000)
    // Access a device register.
    return SetRXWord16Low(rxAddr - REGMEM_BASE, value);
  else if (rxAddr < 0x8000)
    // First 32k of memory - we can use the window, which we currently assume is set to 0.
    return SetRXWord16Window(rxAddr, value);
  else if (rxAddr >= (REGMEM_BASE + APE_OFFSET) && rxAddr < 0xC0002000)
    // APE, etc.
    return SetRXWord16High(rxAddr - (REGMEM_BASE + APE_OFFSET), value);
  else
    // RX CPU SRAM.
    return SetRXWord16ViaForcedStore(rxAddr, value);
#else
  Store16((void*)rxAddr, value);
#endif
}

#ifdef OTG_HOST
static inline uint32_t GetAPEWordViaF8(uint32_t apeAddr) {
  if (apeAddr < 0x00100000)
    abort();

  SetReg(0x00F8, apeAddr - 0x00100000);
  return GetReg(0x00FC);
}

static inline void SetAPEWordViaF8(uint32_t apeAddr, uint32_t v) {
  if (apeAddr < 0x00100000)
    abort();

  SetReg(0x00F8, apeAddr - 0x00100000);
  SetReg(0x00FC, v);
}

static inline uint32_t GetAPEWord(uint32_t apeAddr) {
  if (apeAddr >= 0x00100000)
    return GetAPEWordViaF8(apeAddr);
  fprintf(stderr, "don't know how to get APE word at 0x%08x\n", apeAddr);
  return 0xFFFFFFFF;
}

static inline void SetAPEWord(uint32_t apeAddr, uint32_t v) {
  if (apeAddr >= 0x00100000)
    return SetAPEWordViaF8(apeAddr, v);
  fprintf(stderr, "don't know how to set APE word at 0x%08x\n", apeAddr);
}
#endif

// Gets a device register (as visible in the PCI MMIO space). This corresponds
// to the register offsets specified in the register manual (e.g. 0x362C is
// STATUS).
static inline uint32_t GetReg(uint32_t regno) {
  // TODO: use k1?
  return GetRXWord(RegNoToRXAddress(regno));
}

// Use this for access to registers that don't change (often), it will use
// nonvolatile accesses and be much more efficient. BCM firmware is CONSTANTLY
// reloading registers which don't change (like a core's PCI function number),
// they probably used a standard macro for all register accesses which used a
// volatile pointer.
static inline uint32_t GetImmutableReg(uint32_t regno) {
  // TODO: use k0?
  return GetRXWordImmutable(RegNoToRXAddress(regno));
  //*(uint32_t*)(((uint8_t*)REGMEM_BASE)+regno);
}

static inline void SetReg(uint32_t regno, uint32_t value) {
  // TODO: use k1?
  return SetRXWord(RegNoToRXAddress(regno), value);
  //*(volatile uint32_t*)(((volatile uint8_t*)REGMEM_BASE)+regno) = value;
}

static inline void MaskReg(uint32_t regno, uint32_t bitsToMask) {
  SetReg(regno, GetReg(regno) & ~bitsToMask);
}

static inline void OrReg(uint32_t regno, uint32_t bitsToOr) {
  SetReg(regno, GetReg(regno) | bitsToOr);
}

static inline void MaskOrReg(uint32_t regno, uint32_t bitsToMask, uint32_t bitsToOr) {
  SetReg(regno, (GetReg(regno) & ~bitsToMask) | bitsToOr);
}

static inline void SetRegBit(uint32_t regno, uint32_t bitsToOr, bool on) {
  SetReg(regno, on ? (GetReg(regno) | bitsToOr) : (GetReg(regno) & ~bitsToOr));
}

static inline uint32_t GetGencom32(uint32_t offset) {
  return GetRXWord(GencomToRXAddress(offset));
}
static inline void SetGencom32(uint32_t offset, uint32_t value) {
  SetRXWord(GencomToRXAddress(offset), value);
}
static inline void SetGencom16(uint32_t offset, uint16_t value) {
  SetRXWord16(GencomToRXAddress(offset), value);
}
#endif /* OTG_APE */

/* ---------------------------------------------------------------- */
// Gencom fields, relative to GENCOM_BASE.
#define GEN_FIRMWARE_MBOX   0x000 /* set to BOOTCODE_READY_MAGIC */
#define GEN_DATA_SIG        0x004 /* set to DRIVER_READY_MAGIC   */
#define GEN_CFG             0x008
  /* [008] Flags:
       Stage1 sets this based on various fields in GEN_CFG_FEATURE and
       GEN_CFG_HW. TODO: Check the below based on stage1, might have gotten it
       confused.

                   0000 0000  00ag D0ix  0f0m 0wnE  AWPP LLqq
                  W: WOL Enable
                  w: WOL Limit 10
                  A: ASF Enable (/MGMT FW Enable)     <- looks like APE enable (bmapi RunAPE)
                  E: EEPROM write-protect           (TG3: EEPROM_WP)
                  m: Mini-PCI
                  f: Fiber WOL
                  x: PXE Expansion ROM Enable
                  g: No GPIO2
                  a: APE Enable
                  n: Reverse N-Way
                  i: Link Idle Power Mode ENable
                  D: Disable Power Saving
                  LL: LED Mode
                    0x0000_0000: LED Mode: MAC
                    0x0000_0004: LED Mode: PHY1
                    0x0000_0008: LED Mode: PHY2
                  qq: Voltage source, from GEN_CFG_HW bits 0:1
                  PP: PHY Type
                    0x0000_0000: PHY Type: Unknown  (TG3: PHY_TYPE_UNKNOWN)
                    0x0000_0010: PHY Type: GPHY     (TG3: PHY_TYPE_COPPER)
                    0x0000_0020: PHY Type: SERDES   (TG3: PHY_TYPE_FIBER)
                    0x20 if SERDES, based on 0x1E4's LSB otherwise
   */
#define GEN_CFG__PHY_TYPE__GPHY   0x00000010
#define GEN_CFG__PHY_TYPE__SERDES 0x00000020
#define GEN_CFG__WOL_ENABLE       0x00000040
#define GEN_CFG__APE_ENABLE       0x00200000
#define GEN_CFG__LED_MODE__SHIFT  2
#define GEN_CFG__LOW2__SHIFT      0
#define GEN_CFG__REVERSE_NWAY     0x00000200
#define GEN_CFG__EEPROM_WP        0x00000100
#define GEN_CFG__EXPANSION_ROM_ENABLE         0x00010000
#define GEN_CFG__WOL_LIMIT_10                 0x00000400
#define GEN_CFG__LINK_IDLE_POWER_MODE_ENABLE  0x00020000
#define GEN_CFG__DISABLE_POWER_SAVING         0x00080000
#define GEN_CFG__FIBER_WOL_CAPABLE            0x00004000
#define GEN_CFG__MINI_PCI                     0x00001000
#define GEN_CFG__ASF_ENABLE                   0x00000080

#define GEN_VER                 0x00C
  /* [00C] Unusual 16-bit field.
   */

#define GEN_PHY_ID              0x024
  /* [024] Can be 0x5A5A5A5A, or MII PHY_IDENTIFIER value.
   */

#define GEN_ASF_STATUS_MBOX     0x0B0
  /* [0B4] NIC_SRAM_FW_ASF_STATUS_MBOX. Sometimes BOOTCODE_READY_MAGIC.
   */

#define GEN_FW_DRV_STATE_MBOX   0x0B4
  /* [0B4]
   */
#define GEN_FW_DRV_STATE_MBOX__START        0x00000001
#define GEN_FW_DRV_STATE_MBOX__START_DONE   0x80000001
#define GEN_FW_DRV_STATE_MBOX__UNLOAD       0x00000002
#define GEN_FW_DRV_STATE_MBOX__UNLOAD_DONE  0x80000002
#define GEN_FW_DRV_STATE_MBOX__WOL          0x00000003
#define GEN_FW_DRV_STATE_MBOX__SUSPEND      0x00000004

#define GEN_FW_RESET_TYPE_MBOX  0x0B8
  /* [0B8]
   */

#define GEN_0BC                 0x0BC
  /* [0BC]      Unknown. Set to 0xFEFE_0009 at S2 start. Possibly boot progress
   *            indicator. Upper16 is always 0xFEFE, possible validity indicator.
   *            Observed lower16 values:
   *              8
   *              9 (stage2 start)
   */

#define GEN_MAC_ADDR_HIGH_MBOX  0x0C4
#define GEN_MAC_ADDR_LOW_MBOX   0x0C8
#define GEN_CFG_0D8             0x0D8
  /* [0D8]      related to fastboot. some fields:
                  0x80       - indicates code already loaded
                  0x10       - If set, S2 will not modify 
                               REG_MISCELLANEOUS_LOCAL_CONTROL.
                               g_noModifyMiscellaneousLocalControlReg
                  0x04       - Going to call this "force WOL". Forces
                               power down of some parts of the chip in S2 init.
                  0x01       - skip main loop init
   */
#define GEN_CFG_0D8__NO_MODIFY_MISCELLANEOUS_CONTROL 0x10
#define GEN_CFG_0D8__FORCE_WOL 0x04
#define GEN_CFG_0D8__SKIP_MAIN_LOOP_INIT 0x01

#define GEN_1DC                 0x1DC
  /* [1DC]      g_unknownInitByte5. Some information from REG_CHIP_ID

   */
#define GEN_WOL_MBOX            0x1E0
  /* [1D0]
                Register manual states 'recommended value' is 0x474C_0000.
   */
#define GEN_WOL_MBOX__SIGNATURE           0x474C0000
#define GEN_WOL_MBOX__DRV_STATE_SHUTDOWN  0x00000001
#define GEN_WOL_MBOX__DRV_WOL             0x00000002
#define GEN_WOL_MBOX__SET_MAGIC_PKT       0x00000004

#define GEN_CFG_FEATURE         0x1E4
  /* [1E4]      g_unknownInitWord7: (T: set in Talos image)
                  (BMAPI/QLMAPI BM/QL_FW_FEATURE_CONFIG)
   pxe            bit 0: WOL Enable
                    QLMAPI: WOL Enable
                    Used in S1MegaUltraInit Part 15.
                  bit 1: PXE Enable.
                    QLMAPI: PXE Enable
                    Controls whether expansion ROM is searched for.
       unobs      bit 2-5:
                    QLMAPI: PXE Speed
                    0: Autonegotiate, 1: 10M HD, 2: 10M FD, 3: 100M HD, 4: 100M FD, 5: 1000M HD, 6: 1000M FD
                  bit 6:
                    QLMAPI: Force PCI mode. If not set, autodetect. Probably legacy.
                    ?
     T(Fun0)      bit 7: ASF Enable
                    QLMAPI: ASF Enable
                    If set, do not init MII at end of stage1 init and do not
                    init MII/EMAC mode in stage2 init.
                  bit 8-11: PXE BAR Size
                    QLMAPI: PXE BAR Size. "Expansion ROM size".
                    Involved in expansion ROM init.
                    0: 64K, 1: 128K, 2: 256K, 3: 512K, 4: 1M, 5: 2M, 6: 4M, 7: 8M, 8: 16M
   pxe            bit 12: Disable Setup Message?
                    QLMAPI: Disable Setup Message. Other source indicates "set to 1 to enable PXE setup prompt".
                    "Disable setup prompt for MBA config"
   pxe            bit 13: Alternate Hotkey Option
                    QLMAPI: Hotkey Option
                    Selects hotkey: Ctrl-S or Ctrl-B.
   pxe unobs      bit 14-15:
                    QLMAPI: PXE Bootstrap Type
                    0: BBS, 1: INT 18h, 2: INT 19h, 3: Disable PXE
   pxe unobs      bit 16-19:
                    QLMAPI: "pxe_timeout_msg"
                    PXE timeout in seconds during PXE setup prompt.
   pxe unobs      bit 20-21:
                    QLMAPI: PXE Boot Protocol
                    "Only referenced by MBA."
                    0: PXE, 1: RPL, 2: BOOTP.
                  bit 22: LOM Design?
                    QLMAPI: LOM Design
                    Enables LSB in unknown reg 0x6550.
       unobs      bit 23-24:
                    QLMAPI: Vaux Cutoff Delay
                    0: 250ms, 1: 150ms, 2: 50ms, 3: 0ms
       unobs      bit 25:
                    QLMAPI: WOL Limit 10
                    1: WOL speed must be limited to 10M only. Else 100M is allowed for WOL.
     T            bit 26:
                    QLMAPI: Link Idle
                    REG_CPMU_CONTROL__LINK_IDLE_POWER_MODE_ENABLE
                    Was "driver_wol_enable" long ago? For that bits 26-27, 0=WOL Disable, 1=Magic Packet WOL, 2=Interesting Packet WOL, 3=Both.
       unobs      bit 27:
                    BMAPI: Reserved
                    QLMAPI: PXE Boot Protocol 1
                    Was "driver_wol_enable" long ago?
       unobs      bit 28:
                    QLMAPI: Cable Sense
   pxe            bit 29:
                    QLMAPI: MBA VLAN Enable
                  bit 30: Link Aware Power Mode
                    QLMAPI: Link Aware
                    Appears to be Link Aware Power Mode.
     T            bit 31: Link Speed Power Mode
                    QLMAPI: Link Speed Power
                    "Link speed power mode."
                    Controls REG_CPMU_CONTROL__LINK_SPEED_POWER_MODE_ENABLE.
   */
#define GEN_CFG_FEATURE__WOL_ENABLE            0x0001
#define GEN_CFG_FEATURE__EXPANSION_ROM_ENABLE  0x0002
#define GEN_CFG_FEATURE__PXE_SPEED__MASK       0x003C
#define GEN_CFG_FEATURE__PXE_BAR_SIZE          0x0F00
#define GEN_CFG_FEATURE__PXE_SPEED__SHIFT      2
#define GEN_CFG_FEATURE__ASF_ENABLE        0x0080
#define GEN_CFG_FEATURE__WOL_LIMIT_10          0x02000000
#define GEN_CFG_FEATURE__LINK_IDLE_POWER_MODE  0x04000000
#define GEN_CFG_FEATURE__LINK_AWARE_POWER_MODE 0x40000000
#define GEN_CFG_FEATURE__LINK_SPEED_POWER_MODE 0x80000000
#define GEN_CFG_FEATURE__LOM_DESIGN            0x00400000

#define GEN_CFG_HW              0x1E8
#define GEN_CFG_2               0x1E8
  /* [1E8]      g_miiInitControlFlags2
                  bit 0-1:
                    QLMAPI: Voltage Source
                    Long ago: 0: 1.3V. 1: 1.8V.
     T=0b01       bit 2-3:
                    QLMAPI: PHY LED Mode
                    Legacy LED control. If GEN_CFG_HW__SHASTA_EXT_LED__LEGACY,
                    the REG_LED_CONTROL__LED_MODE field is set from this field.
                  bit 4-5:
                    QLMAPI: PHY Type. Now disused?
                  bit 6:
                    QLMAPI: Forced Max PCI Retry
                  bit 7-9:
                    QLMAPI: Max PCI Retry
                  bit 10-11:
                    QLMAPI: Dual MAC Mode
                  bit 12:
                    QLMAPI: Reverse N-Way
                    S2: something to do with VMAIN?
                    "Power saving auto-negotiation mode".
                    0: N-way negotiation (1000->100->10).
                    1: Reverse N-way negotiation (10->100->1000).
                  bit 13:
                    QLMAPI: Mini PCI
                    Used by S2.
     T            bit 14:
                    Auto powerdown enable.
                    QLMAPI: Enable Auto Powerdown
                    TG3: NIC_SRAM_DATA_CFG_2_APD_EN
                  bit 15-16:
                    QLMAPI: Shasta Ext LED Mode
                      0b00: SHASTA_EXT_LED_LEGACY
                      0b01: SHASTA_EXT_LED_SHARED
                      0b10: SHASTA_EXT_LED_MAC
                      0b11: SHASTA_EXT_LED_COMBO
                  bit 17:
                    QLMAPI: Capacitative Coupling
                    Controls GRC Mode Control TIME_SYNC_MODE_ENABLE.
                  bit 18:                                     \ bit 18-19:
                    QLMAPI: TX SERDES Override                |   Copied to REG_EAV_REF_CLOCK_CONTROL__TIMESYNC_GPIO_MAPPING.
                  bit 19:                                     |
                    QLMAPI: Reserved "Used to be clkreq"      /
                  bit 20:                                     \ bit 20-22:
                    QLMAPI: Reserved "Used to be ASPM_L0"     |   Copied to REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_0_MAPPING.
                  bit 21:                                     |
                    QLMAPI: Reserved "Used to be ASPM_L1"     |
                  bit 22:                                     /
                  bit 23-25:
                    Copied to REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_1_MAPPING.
                  bit 26-28:
                    Copied to REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_2_MAPPING.
                  bit 29-31:
                    Copied to REG_EAV_REF_CLOCK_CONTROL__APE_GPIO_3_MAPPING.
   */
#define GEN_CFG_HW__REVERSE_NWAY                0x00001000
#define GEN_CFG_HW__MINI_PCI                    0x00002000
#define GEN_CFG_HW__AUTO_POWERDOWN_ENABLE       0x00004000
#define GEN_CFG_HW__LEGACY_LED_MODE__MASK        0x0000000C
#define GEN_CFG_HW__LEGACY_LED_MODE__SHIFT       2
#define GEN_CFG_HW__SHASTA_EXT_LED__LEGACY       0x00000000
#define GEN_CFG_HW__SHASTA_EXT_LED__SHARED       0x00008000
#define GEN_CFG_HW__SHASTA_EXT_LED__MAC          0x00010000
#define GEN_CFG_HW__SHASTA_EXT_LED__COMBO        0x00018000
#define GEN_CFG_HW__SHASTA_EXT_LED__MASK         0x00018000
#define GEN_CFG_HW__TIME_SYNC_MODE_ENABLE        0x00020000
#define GEN_CFG_HW__TIMESYNC_GPIO_MAPPING__MASK  0x000C0000
#define GEN_CFG_HW__TIMESYNC_GPIO_MAPPING__SHIFT 18
#define GEN_CFG_HW__APE_GPIO_0_MAPPING__MASK  0x00700000
#define GEN_CFG_HW__APE_GPIO_0_MAPPING__SHIFT 20
#define GEN_CFG_HW__APE_GPIO_1_MAPPING__MASK  0x03800000
#define GEN_CFG_HW__APE_GPIO_1_MAPPING__SHIFT 23
#define GEN_CFG_HW__APE_GPIO_2_MAPPING__MASK  0x1C000000
#define GEN_CFG_HW__APE_GPIO_2_MAPPING__SHIFT 26
#define GEN_CFG_HW__APE_GPIO_3_MAPPING__MASK  0xE0000000
#define GEN_CFG_HW__APE_GPIO_3_MAPPING__SHIFT 29
#define GEN_CFG_HW__LOW2__MASK   0x00000003
#define GEN_CFG_HW__LOW2__SHIFT  0


#define GEN_CFG_SHARED          0x1EC
#define GEN_CFG_3               0x1EC
  /* [1EC]      g_miiInitControlFlags3, shared config, set from NVM 0x0DC:
                                          This is BM_NIC_SHARED_CONFIG

                bit 0:
                  BMAPI, QLMAPI: portSwap.
                  Not implemented?
                  Causes NCSI clock output disable?
                bit 1:
                  BMAPI: L1ASPM_Debounce_En
                  TG3: NIC_SRAM_ASPM_DEBOUNCE
                bit 2:
                  Fiber WoL Capable   ?
                  BMAPI: FiberWoLCapable
                bit 3:
                  BMAPI: DisablePowerSaving
                  Used in S2?
                bit 4-6:
                  QLMAPI: HotPlugPwrBdgtCnt
                bit 7:
                  QLMAPI: Reserved
                  BMAPI: TODO
                  Set REG_GPHY_CONTROL_STATUS__SWITCHING_REGULATOR_POWER_DOWN.
                bit 8:
                  QLMAPI: Reserved.
                  Set REG_TOP_LEVEL_MISCELLANEOUS_CONTROL_1__NCSI_CLOCK_OUTPUT_DISABLE.
     T=0b01     bit 9-10:
                  QLMAPI: Reserved
                  If 0b01, causes certain MII write (S1MegaUltraInit part 16).
                bit 11:
                  QLMAPI: GPIO0 Output Enable
                bit 12:
                  QLMAPI: Reserved
                bit 13:
                  QLMAPI: GPIO2 Output Enable
                bit 14:
                  QLMAPI: GPIO0 Output Value
     T          bit 15:
                  Reserved
                  Unknown, controls unknown reg 0x64DC bit 0x01 in S1 init.
                bit 16:
                  QLMAPI: GPIO2 Output Value
                bit 17:
                  QLMAPI: NCSI Package ID Method
                bit 18-19:
                  QLMAPI: NCSI Package ID
                bit 20:
                  Guessed: NCSI Enable SMBus.
                  QLMAPI: NCSI BMC Con Method
                    0: RMII
                    1: SMBus
                bit 21:
                  QLMAPI: NCSI SMBus Speed
                    0: 100 kHz
                    1: 400 kHz
     T          bit 22:
                  QLMAPI: Link Flap Avoidance
                  TG3: NIC_SRAM_LNK_FLAP_AVOID
     T          bit 23:
                  QLMAPI: Aux 1G Support
                  TG3: NIC_SRAM_1G_ON_VAUX_OK
                bit 24:
                  QLMAPI: Compliance Mode
                bit 25:
                  QLMAPI: LTR Enable
                bit 26-27:
                  QLMAPI: TPH Features
                  Unknown, used in S1MegaUltraInit part 15.
                bit 28-29:
                  Unknown, used in S1MegaUltraInit part 27.
                  Sets unknown PCIe control reg 0x7C04 (IBM or Huawei subsystems only).
                  Determines number of functions to enable?
                  QLMAPI: Reserved
                bit 30-31:
                  Unknown, used in S1MegaUltraInit part 16.
                  QLMAPI: Reserved
    */
#define GEN_CFG_SHARED__FIBER_WOL_CAPABLE     0x00000004
#define GEN_CFG_SHARED__DISABLE_POWER_SAVING  0x00000008
#define GEN_CFG_SHARED__NCSI_BMC_CON_METHOD__RMII    0x00000000
#define GEN_CFG_SHARED__NCSI_BMC_CON_METHOD__SMBUS   0x00100000  /* if set, use SMBus for NCSI */
#define GEN_CFG_SHARED__NCSI_BMC_CON_METHOD__MASK    0x00100000
#define GEN_CFG_SHARED__LNK_FLAP_AVOID        0x00400000
#define GEN_CFG_SHARED__1G_ON_VAUX_OK         0x00800000

#define GEN_CFG_FEATURE_OTHERFUNCS 0x1F0
  /* [1F0]       This is set to the bitwise OR of the Feature Config words for
   *             all of the functions other than this function. Initialized in
   *             S1MegaUltraInit. Guessed.
   */
#define GEN_CFG_HW_OTHERFUNCS 0x1F4
  /* [1F4]      This is set to the bitwise OR of the Hardware Config words for
   *            all of the functions other than this function. Initialized in
   *            S1MegaUltraInit. Guessed.
   */

#define GEN_FW_VER          0x214
  /* [214]      Set from NVM 0x096 (FW Version, and upper 16 bits of manufacturing date, WTF.)
   */

#define GEN_CFG_HW_2            0x2A8 /* guessing */
#define GEN_CFG_2A8             0x2A8
  /* [2A8]      g_miiInitControlFlags:
   *              0x0001  - Enable autonegotiation in MII init (S2)
                  0x0002  - controls whether expansion ROM is searched for (S1)
                  0x0004  - controls... something
                  0x0030  - S2: Controls some writes to SERDES block 0x10 reg 0x10
                            Possible values:
                              0x00 - ...
                              0x10 - ...
                              0x20 - ...
                              0x30 - ...
                  0x00C0  - S2: Controls some writes to SERDES block 0x13 reg 0x12
                            Possible values:
                              0x00 - ...
                              0x40 - ...
                              0x80 - ...
                              0xC0 - ...
                  0x4000  - S2: Enable some unknown flag in SERDES MII init path 
                            (block 0x10, reg 0x10, bit 0x10)
                  0x2000  - S2: Enable some unknown flag in SERDES MII init path
                            (block 0x10, reg 0x10, bit 0x01)
   */
#define GEN_CFG_HW_2__ENABLE_AUTONEGOTIATION  0x0001
#define GEN_CFG_HW_2__EXPANSION_ROM_ENABLE    0x0002
#define GEN_CFG_2A8__UNKNOWN_SERDES_13_10__MASK  0x0030
#define GEN_CFG_2A8__UNKNOWN_SERDES_13_12__MASK  0x00C0
#define GEN_CFG_2A8__UNKNOWN_SERDES_10_10__10    0x4000
#define GEN_CFG_2A8__UNKNOWN_SERDES_10_10__01    0x2000

#define GEN_CFG_HW_2_OTHERFUNCS 0x2AC
  /* [2AC]      This is set to the bitwise OR of the Hardware Config 2 words for
   *            all of the functions other than this function. Initialized in
   *            S1MegaUltraInit. Guessed.
   */

#define GEN_CPMU_STATUS         0x2B0
#define GEN_CFG_5               0x2BC
  /* [2BC]      g_unknownInitWord3. Set from NVM 0x21C.
                  0x0002: TG3 NIC_SRAM_DISABLE_1G_HALF_ADV
                  0x0001: Sets bit 24 in unknown PCIe control register 0x7C00
                  0x000C: controls init of reg 0x64DC
   */
#define GEN_CFG_5__DISABLE_1G_HALF_ADV  0x0002

// Area for use by otgdbg for injection purposes, doesn't seem to ever be used
// so this should be OK.
#define GEN_DBG_SCRATCH_BEGIN  0x300
#define GEN_DBG_SCRATCH_END    0x3FF

/* ---------------------------------------------------------------- */
#define APE_RCPU_MAGIC 0x52435055 /* 'RCPU' */
#define APE_APE_MAGIC  0x41504521 /* 'APE!' */
#define APE_PROT_MAGIC 0x50524F54 /* 'PROT' */
#define APE_HOST_MAGIC 0x484F5354 /* 'HOST' */
#define APE_NCSI_MAGIC 0x4E435349 /* 'NCSI' */

// Codenames:
//   Sawtooth  5717
//   Cotopaxi  5719
//   Snaggle   5720

// From libbmapi.so:
//   (this may be for an old NIC but it seems to check for
//    the 5719, so)
//
//   APE_REG(0) bit 0x02: APE is halted if set
//   Lib halts APE (HaltAPE) by ORing 0x02 into this
//   word. Twice for some reason.
//
// To reset APE:
//   First HaltAPE.
//   Then RunAPE:
//
// APE external interrupts:
//   Note: These appear to be numbered relative to the first
//   external interrupt (0x10).
//
//   0x02: Unknown, complex, generates 0xF4, 0xEB
//   0x08: H2B, generates log message 0x08
//           N.B. Diag tool refers to this as INT 0x0D, but that's clearly wrong
//   0x0A: Unknown, generates log messages 0x0A, 0xEB. based on port number?
//   0x0E: SMBus Bus0
//   0x10: SMBus Bus1
//   0x11: RMU Egress
//   0x14: General Status Change (Occurs when PCIe link status change/a function GRC is reset/"Ds" changes?)
//   0x18: Vmain/Vaux status change
//   0x19: Link Status Change Ports 0 and 2
//   0x1A: Link Status Change Ports 1 and 3
//   0x0B: Port RX Packet Ports 0 and 2
//   0x1B: Port RX Packet Ports 1 and 3

// --- APE REGISTERS ---

// [0x0000]  APE Mode
// NOTE: Inferred from reversing diagnostic tools. ROUGH.
//
//  APE Diag Image Launch:
//    APE_REG(0) |= HALT|FASTBOOT
//    Copy image into scratchpad memory
//      (0xD800? Is this even the high scratchpad, or something in RX CPU
//       lowmem?)
//    SetReg(REG_APE__GPIO_MSG, 0xD800|0x10_0002); // 0x10_D802
//      The 2 here is probably a red herring/instruction encoding indicator.
//      Should be odd for Thumb, does it indicate non-Thumb...?
//    MaskReg(0, HALT);
//    APEEventExchange('R' (0x52), 0, 0);
//      This doesn't use the event mechanism... actually all it does is set RESET (reset entire APE block).
//
//  APE Diag "kick start":
//    SetReg(REG_APE__GPIO_MSG, 0x102020); // <-- what's 0x102020? Is this an APE code memory address?
//    OrReg(REG_APE__MODE, HALT|FAST_BOOT);
//    MaskReg(REG_APE__MODE, HALT);
#define REG_APE__MODE             APE_REG(0x0000)
//    bit 0x01: Reset APE Block
#define REG_APE__MODE__RESET          0x00000001
//    bit 0x02: APE is halted if set
//      Reset APE CPU by setting this bit then unsetting it again.
//      libbmapi halts APE (HaltAPE) by ORing 0x02 into this word,
//      twice for some reason, then masking it out again (RunAPE),
//      again twice.
//
//    Setting/clearing bit 0x02 resets the APE "CPU"; setting bit 0x01 resets
//    the APE "block", apparently.
#define REG_APE__MODE__HALT           0x00000002
//    bit 0x04: Fast Boot
#define REG_APE__MODE__FAST_BOOT      0x00000004
//    bit 0x08: "HostDiag". ???
#define REG_APE__MODE__HOST_DIAG      0x00000008
//    bit 0x20: "event1"
#define REG_APE__MODE__EVENT_1        0x00000020
//    bit 0x40: "event2"
#define REG_APE__MODE__EVENT_2        0x00000040
//    bit 0x80: "GRCint"
#define REG_APE__MODE__GRC_INT        0x00000080
//   bit 0x0000_0100: Virtual UART? (guessed)
#define REG_APE__MODE__VIRTUAL_UART   0x00000100
//    (more of these can be found in the diagnostic utilities, but they don't
//     seem too interesting)
//   bit 0x0000_0200: "swapATBdw"
#define REG_APE__MODE__SWAP_ATB_DWORD    0x00000200
//   bit 0x0000_0400: "rvrsATBbyte"
#define REG_APE__MODE__REVERSE_ATB_BYTE  0x00000400
//   bit 0x0000_0800: "swapARBdw"
#define REG_APE__MODE__SWAP_ARB_DWORD     0x00000800
//   bit 0x0000_1000: "rvrsARBByte"
#define REG_APE__MODE__REVERSE_ARB_BYTE  0x00001000
//   bit 0x0000_2000: Unknown, set by diag tools when loading bootcode
//     after RX CPU halt
#define REG_APE_MODE__CHANNEL0_2_STATUS   0x0000C000
//   bit 0x0000_4000 \_ set by APE firmware when channel 0 or 2 is up?
//   bit 0x0000_8000 /
#define REG_APE_MODE__CHANNEL1_3_STATUS   0xC0000000
//   bit 0x4000_0000 \_ set by APE firmware when channel 1 or 3 is up?
//   bit 0x8000_0000 /
#define REG_APE__MODE__MEMORY_ECC     0x00040000
//   bit 0x0008_0000: "~IcodePipRd" (active low)
#define REG_APE__MODE__ICODE_PIP_RD_DISABLE   0x00080000

// [0x0004]  APE Status. (tg3: "APE Event". ?)
#define REG_APE__STATUS           APE_REG(0x0004)
#define REG_APE__STATUS__BOOT_STATUS_A__MASK  0xF0000000
#define REG_APE__STATUS__BOOT_STATUS_A__SHIFT 28
  // [ 0] = ""
  // [ 1] = "NMI Exception"
  // [ 2] = "Fault Exception"
  // [ 3] = "Memory Check"
  // [ 4] = "Unknown 4" (yes, Broadcom named it this)
  // [ 5] = "Romloader Disabled"
  // [ 6] = "Magic Number"
  // [ 7] = "APE Init Code"
  // [ 8] = "Header Checksum"
  // [ 9] = "APE Header"
  // [10] = "Image Checksum"
  // [11] = "NVRAM Checksum"
  // [12] = "Invalid Type"
  // [13] = "ROM Loader Checksum"
  // [14] = "Invalid Unzip Len"
  // [15] = "Unknown F"
#define REG_APE__STATUS__BOOT_STATUS_B__MASK  0x0F000000
#define REG_APE__STATUS__BOOT_STATUS_B__SHIFT 24
  // [ 0] = "Prog 0"
  // [ 1] = "Prog 1"
  // [ 2] = "BPC Enter"
  // [ 3] = "Decode"
  // [ 4] = "Read NVRAM Header"
  // [ 5] = "Read Code"
  // [ 6] = "Jump"
  // [ 7] = "Prog 7"
  // [ 8] = "BPC Success"

// These Dstate fields indicate the port is in D3 if set.
// Otherwise the port is in D0-D2.
#define REG_APE__STATUS__LAN_0_DSTATE         0x00000010 /* might actually be W2C has-changed indicator */
#define REG_APE__STATUS__LAN_1_DSTATE         0x00000200 /* might actually be W2C has-changed indicator */
// 1: Fast boot mode. 0: NVRAM boot mode.
#define REG_APE__STATUS__BOOT_MODE            0x00000020
#define REG_APE__STATUS__PCIE_RESET           0x00000001
#define REG_APE__STATUS__LAN_0_GRC_RESET      0x00000002 /* might be W2C has-happened indicator */
#define REG_APE__STATUS__LAN_1_GRC_RESET      0x00000080 /* might be W2C has-happened indicator */
#define REG_APE__STATUS__NVRAM_CONTROL_RESET  0x00000008

#define REG_APE__GPIO_MSG         APE_REG(0x0008)
  // dstBase|0x10_0002
  // Set to 0x10_2020 as part of the "kick start" procedure.
  // Set to 0x10_D802 after writing APE image to scratchpad...
  //
  // Feels like this has something to do with fastboot and that
  // the value of it is an APE memory address in the code region.
  //
  // Low 2 bits:
  //   0b10:  Fast boot (required for REG_APE__MODE__FAST_BOOT to work)
  //           If REG_APE__MODE__FAST_BOOT is set:
  //             If this is not 0b10, ROM hangs.
  //             If this is 0b10, low 2 bits are masked, used as reset
  //             vector. REG_APE__GPIO_MSG is modified to mask the low
  //             two bits.
  //

// [0x000C]  APE Event
#define REG_APE__EVENT            APE_REG(0x000C)
#define REG_APE__EVENT__1         0x0000001

// [0x0014]  Series APE+0x14 RXBufOffset - Func0
//   Examined on APE Packet RX interrupt.
//   Appears to indicate incoming packet. Probably references offset into 0xA000_0000.
//
//   bit 30: Packet available to read?
//   INT 0Bh handler / diag tool logdump appears to indicate this contains
//   buffer head/tail/count:
//     bits  0:11: tail
//     bits 12:23: head
//     bits 24:25: ?
//     bits 26:29: count
//     bit     30: indicates validity/packet is available to read
//     bit     31: ?
//
//   r14x
//             "larq_dq": "bufCnt", "head", "tail", flags ipFrag, toHost
//             After receiving from network, if bit 30 is set, bit 31 is ORed in.
//             Indicates some sort of conclusion...

// [0x0018]  Series APE+0x14 RXBufOffset - Func1
//   See Func0. Same format.

// [0x001C]  Series APE+0x1C "TX To Network Doorbell" - Func0
//   Relates to Func0.
//   Written on APE TX to network after filling 0xA002 buffer with packet.
//     bits  0:11: prob tail -- set to offset in 0xA002 buffer to which
//                   LAST block of outgoing frame has just been written
//     bits 12:23: prob head -- set to offset in 0xA002 buffer to which
//                   FIRST block of outgoing frame has just been written
//     bits 24:31: probably length. in bytes, I think
//   CONFIRMED via testing, pretty much

// [0x008C]  Series APE+0x8C Unknown - Func0
// [0x0110]  Series APE+0x8C Unknown - Func1
// [0x0220]  Series APE+0x8C Unknown - Func2
// [0x0320]  Series APE+0x8C Unknown - Func3

// [0x0090]  Series APE+0x90 "APE TX To Net Buffer Allocator" - Func0
// [0x0114]  Series APE+0x90 "APE TX To Net Buffer Allocator" - Func1
// [0x0224]  Series APE+0x90 "APE TX To Net Buffer Allocator" - Func2
// [0x0324]  Series APE+0x90 "APE TX To Net Buffer Allocator" - Func3
//   The low 12 bits of this seem to be an offset into the 0xA002 buffer
//   which determines where the next packet TX'd to the network is
//   written by the APE code. The low 10 bits are multiplied by 128
//   to get the offset.
//
//   USAGE:
//     Write 0x0000_1000 to this register.
//     Poll the register until bits 13:14 are not 0b01 -- busy wait until allocation complete.
//     The block number within the 0xA002_0000 buffer is now in bits 0:11.
//     Blocks are 128 bytes in size, so e.g. if bits 0:11 = 0x024, write to 0xA002_1200.
//     If a frame is more than one block in size, allocate a block, copy to it, repeat until
//     frame is done.
//     Finally finish it all off by writing to the Series APE+1C doorbell with length,
//     first block no. and final block no.
//

// [0x0094]  Series APE+0x94 Unknown - Func0
// [0x0118]  Series APE+0x94 Unknown - Func1
// [0x0228]  Series APE+0x94 Unknown - Func2
// [0x0328]  Series APE+0x94 Unknown - Func3

// [0x0098]  Series APE+0x98 Unknown - Func0
// [0x011C]  Series APE+0x98 Unknown - Func1
// [0x022C]  Series APE+0x98 Unknown - Func2
// [0x032C]  Series APE+0x98 Unknown - Func3

// Note: tg3 driver uses PER_LOCK registers instead of these when the device is
// not a 5761. Possibly these are obsolete.
// 
// Looks like these registers were moved elsewhere when tg3 was expanded to support
// third/fourth ports.
//#define REG_APE__LOCK_REQ         APE_REG(0x002C)
//#define REG_APE__LOCK_GRANT       APE_REG(0x004C)

// [0x0020]  Series APE+0x20 Unknown - Func0
// [0x0024]  Series APE+0x20 Unknown - Func1

// [0x0028]  Unknown Mysterious
// Observed: 0x6022_0000, which is Func0 APE SHM

// [0x002C]  APE Mode 2
#define REG_APE__MODE_2           APE_REG(0x002C)
// Expansion for REG_APE__MODE_2, looks like these fields scale with the number
// of ports.

// [0x0030]  APE Status 2
// Need to double-check these.
#define REG_APE__STATUS_2         APE_REG(0x0030)
#define REG_APE__STATUS_2__LAN_2_GRC_RESET  0x0002 /* might be W2C has-happened indicator */
#define REG_APE__STATUS_2__LAN_3_GRC_RESET  0x0100 /* might be W2C has-happened indicator */
#define REG_APE__STATUS_2__LAN_2_DSTATE     0x0004 /* might be W2C has-changed indicator */
#define REG_APE__STATUS_2__LAN_3_DSTATE     0x0080 /* might be W2C has-changed indicator */

// [0x0040]  Series APE+0x40 Func0
//   Written with info when finished writing a frame to the network.
//   Indicates end of transmission? Weird since actual data written to
//   0xA008_0400 (for F0).
// [0x0044]  Series APE+0x44 Func0
// [0x0048]  Series APE+0x40 Func1
// [0x004C]  Series APE+0x4C Func1

// [0x0054]  Unknown, bit 0 checked by INT H2B
//   bit     0: Not Empty
//   bit     1: Underflow
//   bit  8:24: Number of ... bytes? words?
//   bit 25:31:

// [0x006C]  Unknown Mysterious
// Observed: 0x6022_1000, which is Func1 APE SHM

// [0x0070]  Unknown Mysterious
// Observed: 0xA000_0000
// [0x0074]  Unknown Mysterious
// Observed: 0xA002_0000

// Grouping these together since they seem to be the same fields for different ports.
// They all relate to the "APE RX Pool" ("ARPM") and "APE TX Pool" ("ATPM").
// [0x0078]  Port 0 APE RX Pool Mode/Status
//             0x00FF_FF00  "fullcnt"
//             0x0000_0040  Reset
//             0x0000_0020  Error
//             0x0000_0010  Empty
//             0x0000_0004  Enable
//             0x0000_0002  Halt Done
//             0x0000_0001  Halt
#define REG_APE__RX_POOL_MODE_STATUS__ENABLE    0x04

// [0x0084]  Port 0-related B - APE RX Pool "free_pt_a" (r89x)
//             "free_pt_a": "head", "tail", ...
// [0x0080]  Port 0-related D - APE RX Pool Ret (r80x)
//             "arpm_ret", "bufCnt", "head", "tail"
//             Used to indicate when the APE is done with a region of the 0xA000_0000 RX pool buffer
//             so that it can be used to receive another frame.
//             bits  0:11: tail
//             bits 12:??: head
//             bits 27:??: count
//
// [0x007C]  Port 1 APE RX Pool Mode/Status
//             See port 0.
// [0x009C]  Port 1-related B - "free_pt_a"
// [0x0018]  Series APE+0x14 RXBufOffset Func1
// [0x0088]  Port 1-related D - "arpm_ret"
//
// [0x0214]  Port 2 APE RX Pool Mode/Status
//             See port 0.
// [0x021C]  Port 2-related B
// [0x0200]  Series APE+0x14 RXBufOffset Func2
// [0x0218]  Port 2-related D
//
// [0x0314]  Port 3 APE RX Pool Mode/Status
//             See port 0.
// [0x031C]  Port 3-related B
// [0x0300]  Series APE+0x14 RXBufOffset Func3
// [0x0318]  Port 3-related D


// [0x00A8]  Unknown, monotonically increasing value.
//   Reading this may have side effects?
// [0x00AC]  Unknown, monotonically increasing value.


// [0x00B0]  "Ticks". Monotonically increasing value.
#define REG_APE__TICKS            APE_REG(0x00B0)

// [0x00B4]  Unknown Mysterious
// Observed: 0xA004_0000, which is devreg for Func0

// TODO: see diag for field info
#define REG_APE__GPIO             APE_REG(0x00B8)
#define REG_APE__GPIO__PIN0_UNKIN        0x00000001
#define REG_APE__GPIO__PIN1_UNKIN        0x00000002
#define REG_APE__GPIO__PIN2_UNKIN        0x00000004
#define REG_APE__GPIO__PIN3_UNKIN        0x00000008
#define REG_APE__GPIO__PIN0_UNKOUT       0x00000100
#define REG_APE__GPIO__PIN1_UNKOUT       0x00000200
#define REG_APE__GPIO__PIN2_UNKOUT       0x00000400
#define REG_APE__GPIO__PIN3_UNKOUT       0x00000800
#define REG_APE__GPIO__PIN0_MODE_OUTPUT  0x00010000
#define REG_APE__GPIO__PIN1_MODE_OUTPUT  0x00020000
#define REG_APE__GPIO__PIN2_MODE_OUTPUT  0x00040000
#define REG_APE__GPIO__PIN3_MODE_OUTPUT  0x00080000

#define REG_APE__GINT             APE_REG(0x00BC)
// TODO: see diag for field info

// [0x00E0] (from diag), port 0, number of these scales with ports
#define REG_APE__XOFF0            APE_REG(0x00E0)

// There should be 256 bits of OTP for the 5719.
#define REG_APE__OTP_CONTROL      APE_REG(0x00E8)
#define REG_APE__OTP_CONTROL__PROG_ENABLE 0x00200000
#define REG_APE__OTP_CONTROL__CMD_READ    0x00000000
#define REG_APE__OTP_CONTROL__UNKNOWN     0x00000008
#define REG_APE__OTP_CONTROL__START       0x00000001

#define REG_APE__OTP_STATUS       APE_REG(0x00EC)
#define REG_APE__OTP_STATUS__CMD_DONE     0x00000001

// This is the address offset IN BITS.
#define REG_APE__OTP_ADDR         APE_REG(0x00F0)
#define REG_APE__OTP_ADDR__CPU_ENABLE     0x80000000

// Can read 32 bits at a time.
#define REG_APE__OTP_READ_DATA    APE_REG(0x00F8)

// [0x0100]  Unknown Mysterious
// Observed: 0xA005_0000, which is devreg for Func1

// [0x0104] (from diag), port 1
#define REG_APE__XOFF1            APE_REG(0x0104)

// [0x0108]  "CM3" (Cortex M3)
#define REG_APE__CM3              APE_REG(0x0108)
// CPU Status Field:
//   0: Running
//   1: Halted
//   2: Locked Out
//   3: Sleeping
//   4: Deep Sleep
//   8: Interrupt Pending
//   9: Interrupt Entry
//  10: Interrupt Exit
//  11: Interrupt Return
//   5, 6, 7, 12, 13, 14, 15: Reserved
#define REG_APE__CM3__CPU_STATUS__MASK   0x0000000F
#define REG_APE__CM3__CPU_STATUS__SHIFT  0
#define REG_APE__CM3__ACTIVE_INTERRUPT__MASK 0xFF000000

// [0x0120]  Series APE+1C Unknown - Func1
//   Relates to Func1.

// [0x0200]  Series APE+0x14 Unknown - Func2
//   See Series APE+0x14 Func0. Same format.

// [0x0204]  Series APE+1C Unknown - Func2
//   Relates to Func2.
// [0x0208]  Series APE+0x20 Unknown - Func2

// [0x0214]  Port 2 APE RX Pool Mode/Status
// [0x0218]  Port 2-related D - Ret
// [0x021C]  Port 2-related B - "free_pt_a"
// [0x0220]  Series APE+0x8C Unknown - Func2

// [0x0238] (from diag), port 2
#define REG_APE__XOFF2            APE_REG(0x0238)

// [0x0300]  Series APE+0x14 Unknown - Func3
// [0x0300]  Port 3-related C

// [0x0304]  Series APE+1C Unknown - Func3
//   Relates to Func3.
// [0x0308]  Series APE+0x20 Unknown - Func3

// [0x0314]  Port 3 APE RX Pool Mode/Status
// [0x0318]  Port 3-related D - Ret
// [0x031C]  Port 3-related B - "free_pt_a"
// [0x0320]  Series APE+0x8C Unknown - Func3

// [0x0338] (from diag), port 3
#define REG_APE__XOFF3            APE_REG(0x0338)


// --- APE SHARED MEMORY ---   @ APE 0x4000  ---------------------------------
// Note: the Function 0 APE SHM is mapped into the APE memory map at
// 0x6022_0000.
//
// The SHM is split into sections which appear to be categorised by theme,
// in particular which CPU writes to that area (APE, host, RX CPU, etc.)

// +++ FW SECTION +++++++++++++++++++++++++++++++++++++++++++++++++++++++++40+
#define REG_APE__SEG_SIG          APE_REG(0x4000)
// APE_APE_MAGIC ('APE!') when all is well

#define REG_APE__SEG_LEN          APE_REG(0x4004)
// This may be <= 0x64 or >0x64.
// This may be <= 0xE4 or >0xE4...
// Note: actual observed length is 0x34.

#define REG_APE__INIT_COUNT       APE_REG(0x4008)
// Incremented by the APE every time it initializes.

#define REG_APE__FW_STATUS        APE_REG(0x400C)
//   read from libbmapi.so IsAPEHalted
//   test: bit 0xF000_0000 set means FW is halted?
//   0x00FF_0000 field:
//     0x00 - err 0
//     0x01 - err creating timer hisr
//     0x02 - err creating timer task
//     0x03 - err stack overflow
//     0x04 - err unhandled interrupt
//     0x05 - not in supervisor mode
//     0x06 - not enough DTLBs
//     0x07 - not enough ITLBs
//     0x08 - err stack underflow
//     0x09 - unhandled exception
//   Note:
//     The top 4 bits (0xF000_0000) are set whenever the APE exception handler
//     is invoked; then the APE hangs in an eternal spin loop.
//     Assuming it's a 4-bit field which I'm going to call HEALTH.
#define REG_APE__FW_STATUS__READY   0x00000100
#define REG_APE__FW_STATUS__HEALTH__HALT    0xF0000000
#define REG_APE__FW_STATUS__HEALTH__MASK    0xF0000000

#define REG_APE__FW_FEATURES      APE_REG(0x4010)
#define REG_APE__FW_FEATURES__NCSI    0x00000002

#define REG_APE__FW_BEHAVIOR      APE_REG(0x4014)
// Diag tools call this "behavior". But it also seems to have contents eerily
// similar to REG_NVM_CONFIG_1. Hmmmmm?
//   0x0200: Possibly Mini PCI flag.
//   0x0100: Indicates Mini PCI flag is valid?

#define REG_APE__FW_VERSION       APE_REG(0x4018)
  // 0xMMmm_rrbb
  // M: major, m: minor, r: revision, b: build

// Specifies the offset of a scratchpad area, relative to the start of the APE
// SHM area (i.e., relative to APE_REG(0x4000).
#define REG_APE__SEG_MSG_BUF_OFF  APE_REG(0x401C)
// Size of the scratchpad area in bytes.
#define REG_APE__SEG_MSG_BUF_LEN  APE_REG(0x4020)

// unknown: 0x4024, bootcode related - heartbeat related
#define REG_APE__HEARTBEAT1         APE_REG(0x4024)

// unknown: 0x4028, bootcode related - heartbeat related
// This seems to increase gradually. Monotonic. Rate of increase almost
// certainly 1 unit per second.
#define REG_APE__HEARTBEAT          APE_REG(0x4028)

// unknown: 0x402C, touched by APE exception handler
//   Seems to be set to the exception number which occurred
//   (0-15: ARM-defined, 16+: external interrupts)
#define REG_APE__EXCEPTION_VECTOR   APE_REG(0x402C)

// unknown: 0x4030, touched by APE exception handler
//   Seems to be set to an argument to the exception handler - exception SP?
#define REG_APE__EXCEPTION_SP       APE_REG(0x4030)

// 0x4074: 0x4154 is set to this on an an exception. "Exception Flags". ?

// 0x4078: Seems to be increased by 256 by IntHandler_Ext14h_GenStatusChange
//         in a delay loop

// ------- Probable end of APE SHM section based on segment length field -------
// Chances are the fields below here (as found in the diagnostic tools) are for
// older hardware, stuff supporting ASF, or so on. Not bothering to document it.

// +++ RX CPU SECTION +++++++++++++++++++++++++++++++++++++++++++++++++++++41+
// This section of the SHM is mutated by the RX CPU.

// Set to APE_RCPU_MAGIC ('RCPU') by RX CPU.
#define REG_APE__RCPU_SEG_SIG     APE_REG(0x4100)
// Known value: 0x34.
#define REG_APE__RCPU_SEG_LEN     APE_REG(0x4104)
// Incremented by RX CPU every boot.
#define REG_APE__RCPU_INIT_COUNT  APE_REG(0x4108)
// Appears to be set to the RX CPU firmware version.
// 0xMMmm. M: major, m: minor. e.g. 0x0127 -> v1.39.
#define REG_APE__RCPU_FW_VERSION  APE_REG(0x410C)
// Set from GEN_CFG_FEATURE (GEN 1E4).
#define REG_APE__RCPU_CFG_FEATURE APE_REG(0x4110)
// PCI Vendor/Device ID. Set by S2 bootcode.
#define REG_APE__RCPU_PCI_VENDOR_DEVICE_ID  APE_REG(0x4114)
// PCI Subsystem Vendor/Subsystem ID. Set by S2 bootcode.
#define REG_APE__RCPU_PCI_SUBSYSTEM_ID      APE_REG(0x4118)

// Unknown. Incremented by frobnicating routine. diag: "ape_rst": "cnt"
#define REG_APE__RCPU_APE_RESET_COUNT        APE_REG(0x411C)
// Unknown. Written by frobnicating routine. diag: "ape_rst": "last_ape_stat"
#define REG_APE__RCPU_LAST_APE_STATUS        APE_REG(0x4120)

// Unknown. REG_APE__FW_STATUS is copied to this by the frobnicating routine.
// diag: "last_ape_fw_status"
#define REG_APE__RCPU_LAST_APE_FW_STATUS    APE_REG(0x4124)

// Set from GEN_CFG_HW (CFG 2, GEN 1E8).
#define REG_APE__RCPU_CFG_HW      APE_REG(0x4128)
// Set from GEN_CFG_HW_2 (GEN 2A8).
#define REG_APE__RCPU_CFG_HW_2    APE_REG(0x412C)
// Set from GEN_CPMU_STATUS (which is the upper 16 bits of REG_STATUS, and
// 0x362C in the lower 16 bits).
#define REG_APE__RCPU_CPMU_STATUS APE_REG(0x4130)

// ------- Probable end of RCPU SHM section based on segment length field ------

// +++ APE EXCEPTION REPORTING SECTION ++++++++++++++++++++++++++++++++++++41+
// This section of the SHM is mutated by the APE when an APE CPU exception
// occurs.

// 0x4140  -- Exception Count Initialization Marker
//   Exception handler sets APE_REG(0x4144) to zero if this isn't set to magic
//   'CRE\0' ('\0ERC'?) and then sets this to that magic.
#define REG_APE__CRE_SEG_SIG    APE_REG(0x4140)
#define CRE_MAGIC               0x43524500  /* "CRE\0" */

// 0x4144  -- Exception Count
//   Set to zero by exception handler if not initialized.
//   Incremented upon each exception.
#define REG_APE__CRE_COUNT      APE_REG(0x4144)

// 0x4148  -- "Last Heartbeat"
//   Set from APE_REG(0x4028) on exception.
#define REG_APE__CRE_LAST_HEARTBEAT  APE_REG(0x4148)

// 0x414C  -- "Last Ticks"
//   Set from APE_REG(0x4060) on exception.
#define REG_APE__CRE_LAST_TICKS      APE_REG(0x414C)

// 0x4150  -- FW Status
//   Set from REG_APE__FW_STATUS on exception.
#define REG_APE__CRE_LAST_FW_STATUS  APE_REG(0x4150)

// 0x4154  -- "Exception Flags"
//   Set from APE_REG(0x4074).
//     'TO' probably means timeout
//     0x0000_0001: RegRepair
//     0x0000_0002: SMBIOSmutexTO
//     0x0000_0004: SHMEMmutexTO
//     0x0000_0008: PHYmutexTO
//     0x0000_0010: CFGmutexTO
//     0x0000_0020: IPv4AddrErr
//     0x0000_0040: GRCRESETmutexTO
//     0x8000_0000: CfgReadErr
//     0x0000_0100: CfgWriteErr
//     0x0000_0200: MIIbusyTO
//     0x0000_0400: IndMapping
//     0x0000_0800: KrbRpCacheFull
//     0x0000_1000: EventWrCacheFull
//     0x0000_2000: RmcpReqErr
//     0x0000_4000: RmcpRespErr
//     0x0000_8000: MacAddrErr
//     0x0001_0000: SslSetupErr
//     0x0002_0000: UninitOTPKey
//     0x0004_0000: CfgVerifyErr
//     0x0040_0000: AgentAbend
//     0x0080_0000: UnexpectedRST
#define REG_APE__CRE_EXCEPTION_FLAGS  APE_REG(0x4154)

// 0x4158  -- "Exception Vector"
//   Set from APE_REG(0x402C).
#define REG_APE__CRE_EXCEPTION_VECTOR APE_REG(0x4158)

// 0x415C  -- "Exception SP"
//   Set from APE_REG(0x4030).
#define REG_APE__CRE_REG_SP           APE_REG(0x415C)

// 0x4160  -- R0   ] Looks like the registers saved by the APE CPU before executing
// 0x4164  -- R1   ] the exception handler.
// 0x4168  -- R2   ]
// 0x416C  -- R3   ]
// 0x4170  -- R12  ]
// 0x4174  -- LR   ]
// 0x4178  -- PC   ]
// 0x417C  -- xPSR ]
#define REG_APE__CRE_REG_R0           APE_REG(0x4160)
#define REG_APE__CRE_REG_R1           APE_REG(0x4164)
#define REG_APE__CRE_REG_R2           APE_REG(0x4168)
#define REG_APE__CRE_REG_R3           APE_REG(0x416C)
#define REG_APE__CRE_REG_R12          APE_REG(0x4170)
#define REG_APE__CRE_REG_LR           APE_REG(0x4174)
#define REG_APE__CRE_REG_PC           APE_REG(0x4178)
#define REG_APE__CRE_REG_XPSR         APE_REG(0x417C)

// 0x4254  -- R4   ] More registers saved by the APE CPU before executing the exception
// 0x4258  -- R5   ] handler. I guess these were added later, so there are probably
// 0x425C  -- R6   ] other, existing fields between these two blocks of SHM.
// 0x4240  -- R7   ]  
// 0x4244  -- R8   ]
// 0x4248  -- R9   ]
// 0x424C  -- R10  ]
// 0x4250  -- R11  ]
#define REG_APE__CRE_REG_R4           APE_REG(0x4254)
#define REG_APE__CRE_REG_R5           APE_REG(0x4258)
#define REG_APE__CRE_REG_R6           APE_REG(0x425C)
#define REG_APE__CRE_REG_R7           APE_REG(0x4240)
#define REG_APE__CRE_REG_R8           APE_REG(0x4244)
#define REG_APE__CRE_REG_R9           APE_REG(0x4248)
#define REG_APE__CRE_REG_R10          APE_REG(0x424C)
#define REG_APE__CRE_REG_R11          APE_REG(0x4250)

#if 0
  // 0x4240  -- R4   ] More registers saved by the APE CPU before executing the exception
  // 0x4244  -- R5   ] handler. I guess these were added later, so there are probably
  // 0x4248  -- R6   ] other, existing fields between these two blocks of SHM.
  // 0x424C  -- R7   ]   (the names for these ones might need a double check)
  // 0x4250  -- R8   ]
  // 0x4254  -- R9   ]
  // 0x4258  -- R10  ]
  // 0x425C  -- R11  ]
  #define REG_APE__CRE_REG_R4           APE_REG(0x4240)
  #define REG_APE__CRE_REG_R5           APE_REG(0x4244)
  #define REG_APE__CRE_REG_R6           APE_REG(0x4248)
  #define REG_APE__CRE_REG_R7           APE_REG(0x424C)
  #define REG_APE__CRE_REG_R8           APE_REG(0x4250)
  #define REG_APE__CRE_REG_R9           APE_REG(0x4254)
  #define REG_APE__CRE_REG_R10          APE_REG(0x4258)
  #define REG_APE__CRE_REG_R11          APE_REG(0x425C)
#endif

// +++ HOST SECTION +++++++++++++++++++++++++++++++++++++++++++++++++++++++42+
// This section of the SHM is mutated by the host.

// Set to APE_HOST_MAGIC ('HOST') to indicate the section is valid.
#define REG_APE__HOST_SEG_SIG     APE_REG(0x4200)
// Set to 0x20.
#define REG_APE__HOST_SEG_LEN     APE_REG(0x4204)
// Incremented by host upon every driver initialization.
#define REG_APE__HOST_INIT_COUNT  APE_REG(0x4208)

// DIAG:     0x01.._....
// ODI:      0x10.._....
// UNDI:     0x12.._....
// UEFI:     0x13.._....
//
// Linux:    0xF0MM_mm00, M: major version, m: minor version.
// Solaris:  0xF4.._....
// FreeBSD:  0xF6.._....
// Netware:  0xF8.._....
#define REG_APE__HOST_DRIVER_ID   APE_REG(0x420C)
#define REG_APE__HOST_DRIVER_ID__TYPE__UNDI   0x12000000
#define REG_APE__HOST_DRIVER_ID__TYPE__LINUX  0xF0000000
#define REG_APE__HOST_DRIVER_ID__TYPE__MASK   0xFF000000
#define REG_APE__HOST_DRIVER_ID__TYPE__SHIFT  24

#define REG_APE__HOST_BEHAVIOR    APE_REG(0x4210)
#define REG_APE__HOST_BEHAVIOR__NO_PHYLOCK  0x00000001 /* tg3: always set */

// tg3: Configures heartbeating. 0 to disable. 5000=5s, etc.
#define REG_APE__HOST_HEARTBEAT_INT_MS  APE_REG(0x4214)
#define REG_APE__HOST_HEARTBEAT_COUNT   APE_REG(0x4218)

#define REG_APE__HOST_DRIVER_STATE      APE_REG(0x421C)
// From tg3.
#define REG_APE__HOST_DRIVER_STATE__START    0x00000001
#define REG_APE__HOST_DRIVER_STATE__UNLOAD   0x00000002
#define REG_APE__HOST_DRIVER_STATE__WOL      0x00000003
#define REG_APE__HOST_DRIVER_STATE__SUSPEND  0x00000004

#define REG_APE__HOST_WOL_SPEED         APE_REG(0x4224)
#define REG_APE__HOST_WOL_SPEED__AUTO        0x00008000 /* tg3 */

// Borrowed area for APEDBG.
#define REG_APE__APEDBG_STATE            APE_REG(0x4260)
#define REG_APE__APEDBG_CMD              APE_REG(0x4264)
#define REG_APE__APEDBG_ARG0             APE_REG(0x4268)
#define REG_APE__APEDBG_ARG1             APE_REG(0x426C)
#define REG_APE__APEDBG_CMD_ERROR_FLAGS  APE_REG(0x4270)
#define REG_APE__APEDBG_CMD_ERROR_FLAGS__EXCEPTION  0x00000001
#define REG_APE__APEDBG_EXCEPTION_COUNT  APE_REG(0x4274)
#define REG_APE__APEDBG_EXCEPTION_IGNORE APE_REG(0x4278)

#define REG_APE__APEDBG_STATE__RUNNING   0xBEEFCAFE
#define REG_APE__APEDBG_STATE__EXITED    0xBEEF0FFE

#define REG_APE__APEDBG_CMD__MAGIC       0x5CAF0000
#define REG_APE__APEDBG_CMD__MAGIC_MASK  0xFFFF0000
#define REG_APE__APEDBG_CMD__TYPE_MASK   0x0000FFFF
#define REG_APE__APEDBG_CMD__TYPE__NONE        0x0000
#define REG_APE__APEDBG_CMD__TYPE__MEM_GET     0x0001
#define REG_APE__APEDBG_CMD__TYPE__MEM_SET     0x0002
#define REG_APE__APEDBG_CMD__TYPE__CALL_0      0x0003
#define REG_APE__APEDBG_CMD__TYPE__RETURN      0x0004


// +++ EVENT SECTION ++++++++++++++++++++++++++++++++++++++++++++++++++++++43+
// From tg3.
#define REG_APE__EVENT_STATUS     APE_REG(0x4300)
#define REG_APE__EVENT_STATUS__DRIVER_EVENT      0x00000010

#define REG_APE__EVENT_STATUS__TYPE__HALT        0x00000100 /* guessed */
#define REG_APE__EVENT_STATUS__TYPE__RESET       0x00000200 /* guessed */
#define REG_APE__EVENT_STATUS__TYPE__STATE_CHANGE      0x00000500
#define REG_APE__EVENT_STATUS__STATE_START       0x00010000
#define REG_APE__EVENT_STATUS__STATE_UNLOAD      0x00020000
#define REG_APE__EVENT_STATUS__STATE_WOL         0x00030000
#define REG_APE__EVENT_STATUS__STATE_SUSPEND     0x00040000
#define REG_APE__EVENT_STATUS__TYPE__0B          0x00000B00 /* unknown type, but we use this for our shellcode */
#define REG_APE__EVENT_STATUS__TYPE__SCRATCHPAD_READ_  0x00000A00 /* seems to be an alias of SCRATCHPAD_READ */
#define REG_APE__EVENT_STATUS__TYPE__SCRATCHPAD_READ   0x00001600
#define REG_APE__EVENT_STATUS__TYPE__SCRATCHPAD_WRITE  0x00001700

// Driver waits for this bit not to be asserted when obtaining 'event lock'.
#define REG_APE__EVENT_STATUS__PENDING           0x80000000

// It looks like the low byte indicates the source of the event.
#define REG_APE__EVENT_STATUS__SOURCE__MASK 0x000000FF
#define REG_APE__EVENT_STATUS__SOURCE__RX_CPU  0x01
  /* 2: not sure WTF this entry is */
#define REG_APE__EVENT_STATUS__SOURCE__DRIVER  0x10
#define REG_APE__EVENT_STATUS__SOURCE__DIAG    0x11
#define REG_APE__EVENT_STATUS__SOURCE__BMAPI   0x12

// Discerned from diagnostic tools. Upper16 seems to be additional meaning,
// possibly type-specific, though Pending is probably standard.
#define REG_APE__EVENT_STATUS__TYPE__MASK   0x0000FF00
#define REG_APE__EVENT_STATUS__TYPE__SHIFT  8

  // diag tools, based on some subset of the EVENT_STATUS bits, need to check this
  // again:
  //   1:  "ShutdownReq"
  //   2:  "ResetReq"
  //   3:  "ConfigChg"
  //   4:  "Msg2APE"
  //   5:  "StateChg"         -- confirmed, #define is present in tg3 driver
  //   6:  "unknown"             All other types have the potential to be vestigial.
  //   7:  "inited"
  //   8:  "SetTime"
  //   9:  "SetWFI"
  //  10:  "ReadMem"
  //  11:  "Ping"
  //  12:  "AgentCmdClear"
  //  13:  "AgentCmdResult"
  //  14:  "AgentHeartbeat"
  //  15:  "AgentShutdown"
  //  16:  "OptionRomStarted"
  //  17:  "LogAddText"
  //  18:  "LogClear"
  //  19:  "GetSensorReading"
  //  20:  "GetStSensor"
  //  21:  "AsfCmd"
  //  22   -- String table ends here but this is "Scratchpad Read" above.
  //  23   -- String table ends here but this is "Scratchpad Write" above.

// This is set to 'PROT' (APE_PROT_MAGIC) on all functions.
// If it is 'PROT', the following fields (MAC0_HIGH/LOW) are valid.
#define REG_APE__PROT_MAGIC       APE_REG(0x4308)
// [0x430C]  Unknown. Initialized to 0x1C. Seems like EVENT/PROT section length.
#define REG_APE__EVENT_SEG_LEN    APE_REG(0x430C)
// [0x4310]  Unknown. Initialized to 1.
#define REG_APE__PROT_MAC0_HIGH   APE_REG(0x4314)
#define REG_APE__PROT_MAC0_LOW    APE_REG(0x4318)
// The EVENT/PROT region is 0x1C bytes long, starting at APE_REG(0x4300).

// +++ UNKNOWN SECTION ++++++++++++++++++++++++++++++++++++++++++++++++++++47+
// This region gets zeroed at APE init. Size 256 bytes. 64 bytes per port.

// [0x4700]  Port 0 stuff
//      00     i=0 Field A
//      04     i=0 Field B
//      08     i=1 Field A
//      0C     i=1 Field B
//     ...
//             i=7 Field A
//             i=7 Field B
// [0x4740]  Port 1 stuff
// [0x4780]  Port 2 stuff
// [0x47C0]  Port 3 stuff
//   Same structure.

// +++ NCSI SECTION +++++++++++++++++++++++++++++++++++++++++++++++++++++++48+

// Set to NCSI_MAGIC ('NCSI') by APE firmware.
//
// NOTE: FUNCTION 0 ONLY.
#define REG_APE__NCSI_SIG       APE_REG(0x4800)

// These fields concern the NCSI "package".
// This field has the "ID" in bits 5-31. Bits 0-4 unknown.
#define REG_APE__NCSI_PACKAGE_ID   APE_REG(0x4804)
#define REG_APE__NCSI_UNK08        APE_REG(0x4808) /* 'rset'. LSbyte  to 1 while resetting? */
#define REG_APE__NCSI_PORT_COUNT   APE_REG(0x480C) /* 5719: 4 */

// These are ASCII strings which occupy a few registers starting here.
// Both of these strings occupy three 32-bit words and have ASCII characters
// packed into them, with unused trailing bytes as zero.
//
// NOTE: FUNCTION 0 ONLY.
#define REG_APE__NCSI_BUILD_TIME     APE_REG(0x4810)
#define REG_APE__NCSI_BUILD_TIME__2  APE_REG(0x4814)
#define REG_APE__NCSI_BUILD_TIME__3  APE_REG(0x4818)
#define REG_APE__NCSI_BUILD_DATE     APE_REG(0x481C) /* 0x6022_081C */
#define REG_APE__NCSI_BUILD_DATE__2  APE_REG(0x4820)
#define REG_APE__NCSI_BUILD_DATE__3  APE_REG(0x4824)

#define REG_APE__NCSI_UNK2C     APE_REG(0x482C)
// Disable flow control?
#define REG_APE__NCSI_UNK2C__FCDIS        (1U<<0) /* set */
// Hardware arbitration?
#define REG_APE__NCSI_UNK2C__HWARB        (1U<<1) /* set */
#define REG_APE__NCSI_UNK2C__DESELECT     (1U<<2) /* unset */
#define REG_APE__NCSI_UNK2C__SELECTED     (1U<<3) /* set */
#define REG_APE__NCSI_UNK2C__READY        (1U<<4) /* set */
  /* bit 5 */
#define REG_APE__NCSI_UNK2C__1G_HALF_ADV  (1U<<6)
  /* bit 7  - set on boot */
  /* bit 8  - masked on boot if SMBus, else enabled on boot if HP */
#define REG_APE__NCSI_UNK2C__CTRL_SEEN    (1U<<9)
#define REG_APE__NCSI_UNK2C__USE_SMBUS    (1U<<10) /* set */
#define REG_APE__NCSI_UNK2C__SMBUS_400KHZ (1U<<11) /* set: 400kHz, unset: 100kHz */
#define REG_APE__NCSI_UNK2C__B2H          (1U<<12) /* guessed */
  /* bit 13  - set on boot if at least one of SMBus BMC addr or NC addr configured */
  /* bit 14:15 */
#define REG_APE__NCSI_UNK2C__NO_LINK_FLAP (1U<<16) /* unset */
#define REG_APE__NCSI_UNK2C__AUX_1G       (1U<<17) /* unset */
/* bit 15: unknown - may be related to temperature monitoring */
/* bit 16: not seen */
/* bit 17: not seen */
/* bit 18: not seen */
/* bit 19: unknown */
/* bit 20: unknown */
/* bit 21: unknown (Huawei related) */
/* bit 22-23: unknown (Huawei related) */

// Bit 30 is set if DriverBusy occurs (APE Lockport 6 lock request timeout)
// Bit 31 is set if APE Lockport MEM request times out
//0x0010_449B

// Don't know what the split is between len/offset in this field.
// If this is zero, DBGLOG support is not compiled into the APE firmware.
// Observed values: nonzero.
//
// Bits 0-15:  Offset in bytes of debug log ring buffer from start of APE 0x4000.
// Bits 16-17: ?
// Bits 18-31: Length of debug log ring buffer in words (number of entries=len/2).
#define REG_APE__NCSI_DBGLOG_LEN_OFFSET       APE_REG(0x4830)

// The index of the last written entry in the debug log array at APE 0x4E00.
// This is an index into that ring buffer, modulo 64.
#define REG_APE__NCSI_DBGLOG_INDEX            APE_REG(0x4834)

// bit 00:03: 0x0_000x  These bits correspond to ports. Link Status Change
// bit 04:07: 0x0_00x0  Set by Voltage Source Change interrupt (EXT 18H).
// bit    08: 0x0_0100  Set by RXEvenPorts interrupt (EXT 0BH)
// bit    09: 0x0_0200  Set by RXOddPorts interrupt (EXT 1BH)
// bit    10: 0x0_0400  Set by RXEvenPorts interrupt (EXT 0BH)
// bit    11: 0x0_0800  Set by RXOddPorts interrupt (EXT 1BH)
// bit    12: 0x0_1000  Set by H2B interrupt (EXT 08H)
// bit    13: 0x0_2000  Set by RMUEgress interrupt (EXT 11H)
// bit    14: 0x0_4000  Set by GenStatusChange interrupt - on PCIe reset (EXT 14H)
//                        Causes a search for an APE_CODE image in main loop
//                        Used for FW update?
// bit    15: 0x0_8000  Set by GenStatusChange interrupt - unknown cond (EXT 14H)
// bit    16: 0x1_0000  UNKNOWN - Set by sub_110F22, handled in main loop
//                        Appears to be called only by AInitSmbusOuter
//                        "SMBus needs init"?
// bit    17: 0x2_0000  UNKNOWN
// bit    18: 0x4_0000  Set by Link Status Change Even Ports (EXT 19H)
// bit    19: 0x8_0000  Set by Link Status Change Odd Ports (EXT 1AH)
// bit 20:23:           Unknown flag, one for each port. Set on entry to main loop
// bit 24:27:           UNKNOWN
// bit 28:31:           [NOT SEEN]
#define REG_APE__NCSI_HOUSEKEEPING     APE_REG(0x4838)

// Bits 0-7 of this are the "BMC address", probably the SMBus address.
//   Observed value: 0x00.
// Bits 8-15 of this are the "NC address", probably the SMBus address of the 5719.
//   Observed value: 0x64.
#define REG_APE__NCSI_BMC_ADDR  APE_REG(0x483C)

// [0x4840] Unknown
// Touched on APE packet RX?
// Controls debug logging in one init function. ???
// Initialised to 0x100 on boot.

// [0x4844] Unknown
// Touched during APE init, might be some obsolete thing from NVM

// [0x4848] Sometimes has temperature and other data written to it

// [0x484C] Unknown

// The APE code copies the contents of REG_CHIP_ID to this word.
//
// NOTE: FUNCTION 0 ONLY.
#define REG_APE__NCSI_CHIP_ID   APE_REG(0x4890)

// APE 0x4900-0x49FF: NCSI "Channel" (Port) 0 stuff
#define NCSI_CH_REG(Ch, Reg) APE_REG(0x4900 + 64*4*(Ch) + (Reg))

// [NCSIPORT+0x00] Unknown. Calling this "Info".
//   bit  0: Enabled
//     This can be modified via NCSI SELECT PACKAGE and NCSI DESELECT PACKAGE
//   bit  1: "txpt"  -- TX passthrough has been enabled by BMC NCSI command
//   bit  2: "ready"    ] These are ORed into this word by the APE
//   bit  3: "init"     ] for all ports at startup.
//   bit  4: "mfilt"
//   bit  5: "bfilt"
//   bit  6: "serdes"
//   bit  7: ?
//   bit  8: "vlan"
//   bit  9: ?        set on NCSI RESET CHANNEL                   <-\
//   bit 10: "B2H"                                                  |
//   bit 11: "B2N"                                                  |
//   bit 12: "EEE"                                                  |
//   bit 13: unknown, but set from GEN_CFG_HW__MINI_PCI...          |-- related, used by APE tx thingy
//   bit 14: "driver"                                               |
//   bit 15: if set, "pDead"                                        |
//   bit 16: ?                                                      |
//   bit 17: ?  [could be 'needs reset'] clred by NCSI RESET CHAN <-/
//   bit 18:
//   bit 19: ?  [determines if NCSI sometimes]
//   bit 20:
//   bit 21:
//   bit 22: ?    ARPMresetted?
#define REG_APE__NCSI_CHANNELN_INFO(Ch)         NCSI_CH_REG(Ch, 0x00)
#define REG_APE__NCSI_CHANNELN_INFO__ENABLED    (1U<< 0)
#define REG_APE__NCSI_CHANNELN_INFO__TXPT       (1U<< 1)
#define REG_APE__NCSI_CHANNELN_INFO__READY      (1U<< 2)
#define REG_APE__NCSI_CHANNELN_INFO__INIT       (1U<< 3)
#define REG_APE__NCSI_CHANNELN_INFO__MFILT      (1U<< 4)
#define REG_APE__NCSI_CHANNELN_INFO__BFILT      (1U<< 5)
#define REG_APE__NCSI_CHANNELN_INFO__SERDES     (1U<< 6)
  /* ? */
#define REG_APE__NCSI_CHANNELN_INFO__VLAN       (1U<< 8)
  /* ? */
#define REG_APE__NCSI_CHANNELN_INFO__B2H        (1U<<10)
#define REG_APE__NCSI_CHANNELN_INFO__B2N        (1U<<11)
#define REG_APE__NCSI_CHANNELN_INFO__EEE        (1U<<12)
#define REG_APE__NCSI_CHANNELN_INFO__MINI_PCI   (1U<<13)
#define REG_APE__NCSI_CHANNELN_INFO__DRIVER     (1U<<14)
#define REG_APE__NCSI_CHANNELN_INFO__PDEAD      (1U<<15)

// [NCSIPORT+0x04] "McId". AEN Management Controller ID, set by
// BMC when sending AEN ENABLE command and used when sending AENs.
#define REG_APE__NCSI_CHANNELN_MCID(Ch)         NCSI_CH_REG(Ch, 0x04)
// [NCSIPORT+0x08] "aen"
//   Set via NCSI ENABLE AEN.
//   bit 0: Enable Link Status Change AEN
//   bit 1: Enable Configuration Required AEN
//   bit 2: Enable Host NC Driver Status Change AEN
#define REG_APE__NCSI_CHANNELN_AEN(Ch)          NCSI_CH_REG(Ch, 0x08)
// [NCSIPORT+0x0C] "bfilt"
//   Set via NCSI ENABLE AEN.
//   bit 0: ARP Packet
//   bit 1: DHCP Client Packet
//   bit 2: DHCP Server Packet
//   bit 3: NetBIOS Packet
//   Remaining bits: probably unused
#define REG_APE__NCSI_CHANNELN_AEN__ARP            (1U<<0)
#define REG_APE__NCSI_CHANNELN_AEN__DHCP_CLIENT    (1U<<1)
#define REG_APE__NCSI_CHANNELN_AEN__DHCP_SERVER    (1U<<2)
#define REG_APE__NCSI_CHANNELN_AEN__NETBIOS        (1U<<3)
#define REG_APE__NCSI_CHANNELN_BFILT(Ch)        NCSI_CH_REG(Ch, 0x0C)
// [NCSIPORT+0x10] "mfilt"
//   bit 0: IPv6 Neighbour Advertisement
//   bit 1: IPv6 Router Advertisement
//   bit 2: DHCPv6 Relay and Server Multicast
//   Remaining bits: probably unused
#define REG_APE__NCSI_CHANNELN_MFILT(Ch)        NCSI_CH_REG(Ch, 0x10)
// [NCSIPORT+0x14] "Setting" (first word)
//   This is the "Link Settings" value from NCSI Set Link.
//   bit 0: Autonegotiation enabled?
//   bit 1: Link Speed 10M enable
//   bit 2: Link Speed 100M enable
//   bit 3: Link Speed 1000M enable
//   bit 4: Link Speed 10G enable
//   bits 5:7: reserved
//   bit 8: Half duplex enable
//   bit 9: Full duplex enable
//   bit 10: Pause capability enable
//   bit 11: Asymmetric pause capability enable
//   bit 12: OEM link settings field valid
//   bit 13: ?
#define REG_APE__NCSI_CHANNELN_SETTING_1(Ch)    NCSI_CH_REG(Ch, 0x14)
// [NCSIPORT+0x18] "Setting" (second word)
//   This is the "OEM Settings" value from NCSI Set Link.
#define REG_APE__NCSI_CHANNELN_SETTING_2(Ch)    NCSI_CH_REG(Ch, 0x18)

// [NCSIPORT+0x1C] "VLAN"
//   Receives VLAN mode from NCSI specification "Enable VLAN" command.
//   0x01 - VLAN only.
//     Only packets tagged with correct VLAN go to NCSI.
//     Nontagged/non-matching packets don't go to NCSI.
//   0x02 - VLAN+non-VLAN.
//     VLAN-tagged packets with correct VLAN
//     and non-tagged packets go to NCSI.
//   0x03 - Any VLAN+non-VLAN.
//     Any VLAN-tagged packet goes to NCSI, no matter the VLAN.
//     Non-tagged packets also go to NCSI.
#define REG_APE__NCSI_CHANNELN_VLAN(Ch)         NCSI_CH_REG(Ch, 0x1C)

// [NCSIPORT+0x24] "altHostMac". This appears to be specific to NCSI MAC3.
//   Lower 16 bits of this word contains upper 16 bits of the MAC.
#define REG_APE__NCSI_ALT_HOST_MAC_HIGH(Ch)     NCSI_CH_REG(CH, 0x24)
// [NCSIPORT+0x28]
//   Lower 16 bits of this word contains mid 16 bits of the MAC.
#define REG_APE__NCSI_ALT_HOST_MAC_MID(Ch)      NCSI_CH_REG(CH, 0x28)
// [NCSIPORT+0x2C]
//   Lower 16 bits of this word contains low 16 bits of the MAC.
#define REG_APE__NCSI_ALT_HOST_MAC_LOW(Ch)      NCSI_CH_REG(CH, 0x2C)

// [NCSIPORT+0x30+16N] Unknown series, 0 <= N < 4
// Contains an index in range [0, 17). Related to g_bss113B04.
// Each entry:
//   4  ui  
//   4  ui  MAC High
//   4  ui  MAC Mid
//   4  ui  MAC Low
// Looks like an index of the number of RMU block MAC slots which have
// been filled for a given port.

// [NCSIPORT+0x34+16N] MAC Address
// Lower 16 bits of this word contains the upper 16 bits of the MAC.
// (Strange MAC representation.)
// The following MACs are supported:
//   0: MAC0            (v)
//   1: MAC1            (v)
//   2: ?
//   3: "altHostMac"    (a)
//
// (v): VLAN ID field exists for this MAC.
// (a): "ALT_HOST_MAC" exists for this MAC (see
//        REG_APE__NCSI_ALT_HOST_MAC_{HIGH,MID,LOW}.)
#define REG_APE__NCSI_CHANNELN_MACN_HIGH(Ch, N)     NCSI_CH_REG(Ch, 0x34+16*(N))
// Lower 16 bits of this word contains the mid 16 bits of the MAC.
#define REG_APE__NCSI_CHANNELN_MACN_MID(Ch, N)      NCSI_CH_REG(Ch, 0x38+16*(N))
// Lower 16 bits of this word contains the lower 16 bits of the MAC.
#define REG_APE__NCSI_CHANNELN_MACN_LOW(Ch, N)      NCSI_CH_REG(Ch, 0x3C+16*(N))

// [NCSIPORT+0x40] Unknown.

// [NCSIPORT+0x70] Nonzero indicates VLAN field is valid?
#define REG_APE__NCSI_CHANNELN_MAC0_VLAN_VALID(Ch)  NCSI_CH_REG(Ch, 0x70)
// 0 <= N < 2
#define REG_APE__NCSI_CHANNELN_MACN_VLAN_VALID(Ch,N)  NCSI_CH_REG(Ch, 0x70+8*(N))
// [NCSIPORT+0x74] "VLAN".
#define REG_APE__NCSI_CHANNELN_MAC0_VLAN(Ch)        NCSI_CH_REG(Ch, 0x74)
// 0 <= N < 2
#define REG_APE__NCSI_CHANNELN_MACN_VLAN(Ch)        NCSI_CH_REG(Ch, 0x74+8*(N))
// [NCSIPORT+0x78] Nonzero indicates VLAN field is valid?
#define REG_APE__NCSI_CHANNELN_MAC1_VLAN_VALID(Ch)  NCSI_CH_REG(Ch, 0x78)
// [NCSIPORT+0x7C] "VLAN".
#define REG_APE__NCSI_CHANNELN_MAC1_VLAN(Ch)        NCSI_CH_REG(Ch, 0x7C)

// [NCSIPORT+0x80] "Status"
//   bit    0: Link up
//   bits 1-4:
//     0: "link_down"
//     1: "10hd"
//     2: "10Fd"
//     3: "100hd"
//     4: "100t4"
//     5: "100fd"
//     6: "1000hd"
//     7: "1000fd"
//     8: "10g"
//  bit 5: Set from MII_REG_CONTROL__AUTO_NEGOTIATION_ENABLE. Set unconditionally in SERDES case.
//  bit 6: Set if autonegotiation is complete.
//  bit 7: NCSI standard says "parallel detection". Not used.
//  bit 8: NCSI standard says reserved.
//  bit 9: Link partner 1000BASE-T full duplex capable.
//  bit 10: Link partner 1000BASE-T half duplex capable
//  bit 11: Link partner 100BASE-T4 capable.
//  bit 12: Link partner 100BASE-TX full duplex capable
//  bit 13: Link partner 100BASE-TX half duplex capable
//  bit 14: Link partner 10BASE-T full duplex capable
//  bit 15: Link partner 10BASE-T half duplex capable
//  bit 16: Set if TX is paused
//  bit 17: Set if RX is paused
//  bit 18: Link partner symmetric pause capable
//  bit 19: Link partner asymmetric pause capable
//  bit 20: SERDES
//  bit 21: OEM Link Speed settings valid
#define REG_APE__NCSI_CHANNELN_STATUS(Ch)       NCSI_CH_REG(Ch, 0x80)
#define REG_APE__NCSI_CHANNELN_STATUS__UP       0x000001
#define REG_APE__NCSI_CHANNELN_STATUS__SERDES   0x100000 /* guessed */
#define REG_APE__NCSI_CHANNELN_STATUS__MODE__LINK_DOWN    (0<<1)
#define REG_APE__NCSI_CHANNELN_STATUS__MODE__10HD         (1<<1)
#define REG_APE__NCSI_CHANNELN_STATUS__MODE__10FD         (2<<1)
#define REG_APE__NCSI_CHANNELN_STATUS__MODE__100HD        (3<<1)
#define REG_APE__NCSI_CHANNELN_STATUS__MODE__100T4        (4<<1)
#define REG_APE__NCSI_CHANNELN_STATUS__MODE__100FD        (5<<1)
#define REG_APE__NCSI_CHANNELN_STATUS__MODE__1000HD       (6<<1)
#define REG_APE__NCSI_CHANNELN_STATUS__MODE__1000FD       (7<<1)
#define REG_APE__NCSI_CHANNELN_STATUS__MODE__10G          (8<<1)
#define REG_APE__NCSI_CHANNELN_STATUS__RX_PAUSED          0x20000 /* guessed */
#define REG_APE__NCSI_CHANNELN_STATUS__TX_PAUSED          0x10000 /* guessed */
#define REG_APE__NCSI_CHANNELN_STATUS__AUTONEG_ENABLE     0x20
#define REG_APE__NCSI_CHANNELN_STATUS__AUTONEG_COMPLETE   0x40

// [NCSIPORT+0x84] "rsetCnt"
#define REG_APE__NCSI_CHANNELN_RESET_COUNT(Ch)  NCSI_CH_REG(Ch, 0x84)
// [NCSIPORT+0x88] "pxe"
// Seems to be set to [REG_APE__HOST_DRIVER_ID]<<8 if UNDI in use.
#define REG_APE__NCSI_CHANNELN_PXE(Ch)          NCSI_CH_REG(Ch, 0x88)
// [NCSIPORT+0x8C] "dropfil"
//   bit 0x10: ?
#define REG_APE__NCSI_CHANNELN_DROPFIL(Ch)      NCSI_CH_REG(Ch, 0x8C)
// [NCSIPORT+0x90] "slink"
//   The contents of CHANNELN_STATUS gets copied to this when AUTONEG_COMPLETE
//   is set.
#define REG_APE__NCSI_CHANNELN_SLINK(Ch)        NCSI_CH_REG(Ch, 0x90)
// [NCSIPORT+0x98] Unknown
//   0x04:  Appears to be HTX2B Enable, sets REG_GRC_MODE_CONTROL__HTX2B_ENABLE
// [NCSIPORT+0x9C] Unknown
//
// [NCSIPORT+0xA0] "dbg"
#define REG_APE__NCSI_CHANNELN_DBG(Ch)          NCSI_CH_REG(Ch, 0xA0)
// [NCSIPORT+0xA4] Unknown
//   Copied from APE+0x00AC. Time of last init?
// [NCSIPORT+0xA8] Unknown
//   Copied from Setting 1 with some bits masked off.

// --- control statistics ---
// [NCSIPORT+0xB0] "ctrlStat": "rx"
#define REG_APE__NCSI_CHANNELN_CTRLSTAT_RX(Ch)  NCSI_CH_REG(Ch, 0xB0)
// [NCSIPORT+0xB4] "ctrlStat": "dropped"
#define REG_APE__NCSI_CHANNELN_CTRLSTAT_DROPPED(Ch)      NCSI_CH_REG(Ch, 0xB4)
// [NCSIPORT+0xB8] "ctrlStat": "typeErr"
#define REG_APE__NCSI_CHANNELN_CTRLSTAT_TYPE_ERR(Ch)     NCSI_CH_REG(Ch, 0xB8)
// [NCSIPORT+0xBC] "ctrlStat": "csumErr"
#define REG_APE__NCSI_CHANNELN_CTRLSTAT_CSUM_ERR(Ch)     NCSI_CH_REG(Ch, 0xBC)
// [NCSIPORT+0xC0] "ctrlStat": "allRx"
#define REG_APE__NCSI_CHANNELN_CTRLSTAT_ALL_RX(Ch)       NCSI_CH_REG(Ch, 0xC0)
// [NCSIPORT+0xC4] "ctrlStat": "tx"
//   Incremented by some function. Could be TX function.
#define REG_APE__NCSI_CHANNELN_CTRLSTAT_TX(Ch)           NCSI_CH_REG(Ch, 0xC4)
// [NCSIPORT+0xC8] "ctrlStat": "aen"
//   Incremented by some function.
#define REG_APE__NCSI_CHANNELN_CTRLSTAT_AEN(Ch)          NCSI_CH_REG(Ch, 0xC8)

// --- egress statistics ---
// [NCSIPORT+0xCC] "egrStat": "rx" (word 1)   -- incremented when word 2 rolls over
#define REG_APE__NCSI_CHANNELN_EGRSTAT_RX_1(Ch)   NCSI_CH_REG(Ch, 0xCC)
// [NCSIPORT+0xD0] "egrStat": "rx" (word 2)
#define REG_APE__NCSI_CHANNELN_EGRSTAT_RX_2(Ch)   NCSI_CH_REG(Ch, 0xD0)
// [NCSIPORT+0xD4] "egrStat": "dropped"
#define REG_APE__NCSI_CHANNELN_EGRSTAT_DROPPED(Ch)   NCSI_CH_REG(Ch, 0xD4)
// [NCSIPORT+0xD8] "egrStat": "chStErr"
#define REG_APE__NCSI_CHANNELN_EGRSTAT_CHST_ERR(Ch)  NCSI_CH_REG(Ch, 0xD8)
// [NCSIPORT+0xDC] "egrStat": "runt"
#define REG_APE__NCSI_CHANNELN_EGRSTAT_RUNT(Ch)      NCSI_CH_REG(Ch, 0xDC)
// [NCSIPORT+0xE0] "egrStat": "jabber"
#define REG_APE__NCSI_CHANNELN_EGRSTAT_JABBER(Ch)    NCSI_CH_REG(Ch, 0xE0)

// --- ingress statistics ---
// [NCSIPORT+0xE4] "ingrStat": "rx"
#define REG_APE__NCSI_CHANNELN_INGRSTAT_RX(Ch)    NCSI_CH_REG(Ch, 0xE4)
// [NCSIPORT+0xE8] "ingrStat": "dropped"
#define REG_APE__NCSI_CHANNELN_INGRSTAT_DROPPED(Ch)    NCSI_CH_REG(Ch, 0xE8)
// [NCSIPORT+0xEC] "ingrStat": "jabber"
#define REG_APE__NCSI_CHANNELN_INGRSTAT_JABBER(Ch)    NCSI_CH_REG(Ch, 0xEC)
// [NCSIPORT+0xF0] "ingrStat": "runt"
#define REG_APE__NCSI_CHANNELN_INGRSTAT_RUNT(Ch)    NCSI_CH_REG(Ch, 0xF0)
// [NCSIPORT+0xF4] "ingrStat": "chStErr"
#define REG_APE__NCSI_CHANNELN_INGRSTAT_CHST_ERR(Ch)    NCSI_CH_REG(Ch, 0xF4)
// [NCSIPORT+0xF8] ??? possibly relayed to dropfil, it is set to this|0x4000

// APE 0x4A00-0x4AFF: NCSI Channel 1 stuff, same as the above block
// APE 0x4B00-0x4BFF: NCSI Channel 2 stuff, same as the above block
// APE 0x4C00-0x4CFF: NCSI Channel 3 stuff, same as the above block

// [0x4D00] Set to 0x1234567
// [0x4D04] Unknown, incremented in APE init
// [0x4D10] Set to &g_short113154? (&g_ringData)
// [0x4D14] ?
// [0x4D18] Set to g_toBmcState

// 0x4D3C end?

// APE 0x4E00: Debug Log Array Entries (array of 64)
//   Each debug log entry is two words:
//     4  ui  Type/Arg
//       Type is low 8 bits, arg is upper 24 bits.
//     4  ui  Timestamp
//       This is copied from APE 0x4028, a heartbeat value.
// APE 0x5000: Debug Log Ends Here
//
// The last entry in the debug log is indexed by REG_APE__DBGLOG_NDX. Another
// entry is generally not appended if the last entry is the same type/arg
// (repeated entry suppression). When appending an entry, the index is
// incremented (modulo 64) and the new entry is written there.
//
// It looks like the timestamp entry might have 2**28 added to it whenever a
// log entry is repeated. If that 4-bit field saturates, another log entry is
// appended.
#define APE_DBGLOG_BASE         APE_REG(0x4E00)
#define APE_DBGLOG_NUM_ENTRIES  64

// --- APE Periperals --------------------------------------------------------
// Note: It seems odd but this could be "program space", some code in the
// stage2 enables "program space" access before twiddling these. But it
// appears to be confirmed that this is the "periperal" range.

// === APE Peripheral Block 0x000 - UART ===============================
// TODO

// APE_REG(0x8000) -definitely- has UART registers at the very
// start, so AM 6024_0000 must be something different.
// Memory map might be a bit random.
//   Known UART regs 0x8000-0x8014

// APEMEM: NVM access registers are here.
//   6024_0000, at 0x00: feels like NVM Command (REG 0x7000)
//   6024_0004, at 0x04: Unknown. Can be set to 1 by ROM.
//   6024_0008, at 0x08: feels like NVM Write (REG 0x7008)
//   6024_000C, at 0x0C: feels like NVM Address (REG 0x700C)
//   6024_0010, at 0x10: feels like NVM Read (REG 0x7010)
//   6024_0020, at 0x20: feels like Software Arbitration (REG 0x7020)
//   6024_0024, at 0x24: feels like NVM Access (REG 0x7024)

// === APE Peripheral Block 0x100 - SMBus Controller 1 =================

// [APEPER+0x100] SMBus Config
#define REG_APE__SMBUS_1_CONFIG       APE_REG(0x8100)
#define REG_APE__SMBUS_1_CONFIG__MSTR_RETRY_COUNT__MASK  0x000F0000
#define REG_APE__SMBUS_1_CONFIG__RESET                   0x80000000
#define REG_APE__SMBUS_1_CONFIG__ENABLED                 0x40000000
#define REG_APE__SMBUS_1_CONFIG__BITBANG                 0x20000000
#define REG_APE__SMBUS_1_CONFIG__ADDR0                   0x10000000
#define REG_APE__SMBUS_1_CONFIG__PROMISCUOUS             0x08000000
#define REG_APE__SMBUS_1_CONFIG__TM_STMP_COUNT           0x10000000 /* Yes, this aliases ADDR0. Is this a typo? */
#define REG_APE__SMBUS_1_CONFIG__ARP_EN1                 0x00000200
#define REG_APE__SMBUS_1_CONFIG__ARP_EN0                 0x00000100
#define REG_APE__SMBUS_1_CONFIG__HW_ARP                  0x00000080

// [APEPER+0x104] SMBUS Time Config
#define REG_APE__SMBUS_1_TIME_CONFIG  APE_REG(0x8104)
#define REG_APE__SMBUS_1_TIME_CONFIG__400KHZ             0x80000000
#define REG_APE__SMBUS_1_TIME_CONFIG__RNDM__MASK         0x7F000000 /* ms */
#define REG_APE__SMBUS_1_TIME_CONFIG__RNDM__SHIFT        24
#define REG_APE__SMBUS_1_TIME_CONFIG__PERIODIC__MASK     0x00FF0000 /* us */
#define REG_APE__SMBUS_1_TIME_CONFIG__PERIODIC__SHIFT    16
#define REG_APE__SMBUS_1_TIME_CONFIG__IDLE__MASK         0x0000FF00 /* us */
#define REG_APE__SMBUS_1_TIME_CONFIG__IDLE__SHIFT        8
// always printed: "slvStretch"

// [APEPER+0x108] SMBUS Address Config
//   Note: each address field is valid if the corresponding VALID bit is set.
//   The address bits should be left-shifted by 1 after extraction - the actual
//   address is always even.
//
//   Observed addresses: ADDR0=0x64 (after <<1), ADDR1/2/3=unset
#define REG_APE__SMBUS_1_ADDR_CONFIG  APE_REG(0x8108)
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_0_VALID   0x00000080
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_0__MASK   0x0000007F
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_0__SHIFT  0
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_1_VALID   0x00008000
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_1__MASK   0x00007F00
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_1__SHIFT  8
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_2_VALID   0x00800000
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_2__MASK   0x007F0000
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_2__SHIFT  16
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_3_VALID   0x80000000
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_3__MASK   0x7F000000
#define REG_APE__SMBUS_1_ADDR_CONFIG__ADDR_3__SHIFT  24

// [APEPER+0x10C] SMBUS MSTR FIFO
#define REG_APE__SMBUS_1_MSTR_FIFO    APE_REG(0x810C)
#define REG_APE__SMBUS_1_MSTR_FIFO__RX_PACKET_COUNT__MASK     0x007F0000
#define REG_APE__SMBUS_1_MSTR_FIFO__RX_PACKET_COUNT__SHIFT    16
#define REG_APE__SMBUS_1_MSTR_FIFO__RX_FIFO_THRESHOLD__MASK   0x00007F00
#define REG_APE__SMBUS_1_MSTR_FIFO__RX_FIFO_THRESHOLD__SHIFT  8

// [APEPER+0x110] SMBUS SLV FIFO
#define REG_APE__SMBUS_1_SLV_FIFO     APE_REG(0x8110)
#define REG_APE__SMBUS_1_SLV_FIFO__RX_PACKET_COUNT__MASK     0x007F0000
#define REG_APE__SMBUS_1_SLV_FIFO__RX_PACKET_COUNT__SHIFT    16
#define REG_APE__SMBUS_1_SLV_FIFO__RX_FIFO_THRESHOLD__MASK   0x00007F00
#define REG_APE__SMBUS_1_SLV_FIFO__RX_FIFO_THRESHOLD__SHIFT  8

// [APEPER+0x114] SMBUS Bit Bang
#define REG_APE__SMBUS_1_BIT_BANG     APE_REG(0x8114)
#define REG_APE__SMBUS_1_BIT_BANG__CLOCK    0x80000000
#define REG_APE__SMBUS_1_BIT_BANG__DATA     0x20000000

// [APEPER+0x130] SMBUS MSTR Command
#define REG_APE__SMBUS_1_MSTR_COMMAND   APE_REG(0x8130)
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__MASK  0x00001E00
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__SHIFT 9
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__QUICK_CMD        0 /* "QuickCmd" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__SEND_BYTE        1 /* "SendByte" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__RECV_BYTE        2 /* "RecvByte" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__WRITE_BYTE       3 /* "WriteByte" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__READ_BYTE        4 /* "ReadByte" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__WRITE_WORD       5 /* "WriteWd" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__READ_WORD        6 /* "ReadWd" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__WRITE_BLOCK      7 /* "BlkWrite" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__READ_BLOCK       8 /* "BlkRead" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__PROC_CALL        9 /* "ProcCall" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__BWBR_PROC_CALL  10 /* "BWBRprocCall" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__TYPE__HOST_NOTIFY     11 /* "HostNotify" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__READ_BYTE_COUNT__MASK   0x000000FF
#define REG_APE__SMBUS_1_MSTR_COMMAND__READ_BYTE_COUNT__SHIFT  0
#define REG_APE__SMBUS_1_MSTR_COMMAND__ST__MASK                0x0E000000
#define REG_APE__SMBUS_1_MSTR_COMMAND__ST__SHIFT               25
#define REG_APE__SMBUS_1_MSTR_COMMAND__ST__OK                 0 /* "OK" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__ST__LOST_ARB           1 /* "lostArb" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__ST__NACK_1             2 /* "nack1" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__ST__NACK_N             3 /* "nackN" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__ST__GT_TOMIN           4 /* ">TOmin" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__ST__GT_TLM5            5 /* ">Tlm5" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__ST__GT_TLM6            6 /* ">Tlm6" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__START_BUSY             0x80000000 /* "startBsy" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__ABORT                  0x40000000 /* "abort" */
#define REG_APE__SMBUS_1_MSTR_COMMAND__PEC                    0x00000100 /* "pec" */

// [APEPER+0x134] SMBUS SLV Command
#define REG_APE__SMBUS_1_SLV_COMMAND   APE_REG(0x8134)
#define REG_APE__SMBUS_1_SLV_COMMAND__ST__MASK                0x03800000
#define REG_APE__SMBUS_1_SLV_COMMAND__ST__SHIFT               23
#define REG_APE__SMBUS_1_SLV_COMMAND__ST__OK                  0     /* "OK" */
#define REG_APE__SMBUS_1_SLV_COMMAND__ST__GT_TOMIN            5     /* ">TOmin" */
#define REG_APE__SMBUS_1_SLV_COMMAND__ST__MSTR_ABORT          7     /* "mstrAbort" */
#define REG_APE__SMBUS_1_SLV_COMMAND__START_BUSY              0x80000000
#define REG_APE__SMBUS_1_SLV_COMMAND__ABORT                   0x40000000
#define REG_APE__SMBUS_1_SLV_COMMAND__PEC                     0x00000100

// [APEPER+0x138] SMBUS Event Enable
#define REG_APE__SMBUS_1_EVENT_ENABLE  APE_REG(0x8138)
#define REG_APE__SMBUS_1_EVENT_ENABLE__MSTR_RX_FIFO_FULL           0x80000000
#define REG_APE__SMBUS_1_EVENT_ENABLE__MSTR_RX_THRESHOLD_HIT       0x40000000
#define REG_APE__SMBUS_1_EVENT_ENABLE__MSTR_RX_EVENT               0x20000000
#define REG_APE__SMBUS_1_EVENT_ENABLE__MSTR_START_BUSY_0           0x10000000
#define REG_APE__SMBUS_1_EVENT_ENABLE__MSTR_TX_UNDERRUN            0x08000000
#define REG_APE__SMBUS_1_EVENT_ENABLE__SLV_RX_FIFO_FULL            0x04000000
#define REG_APE__SMBUS_1_EVENT_ENABLE__SLV_RX_THRESHOLD_HIT        0x02000000
#define REG_APE__SMBUS_1_EVENT_ENABLE__SLV_RX_EVENT                0x01000000
#define REG_APE__SMBUS_1_EVENT_ENABLE__SLV_START_BUSY_0            0x00800000
#define REG_APE__SMBUS_1_EVENT_ENABLE__SLV_TX_UNDERRUN             0x00400000
#define REG_APE__SMBUS_1_EVENT_ENABLE__SLV_READ_EVENT              0x00200000
#define REG_APE__SMBUS_1_EVENT_ENABLE__SLV_ARP_EVENT               0x00100000

// [APEPER+0x13C] SMBUS Event St
// This register is W2C.
#define REG_APE__SMBUS_1_EVENT_ST      APE_REG(0x813C)
#define REG_APE__SMBUS_1_EVENT_ST__MSTR_RX_FIFO_FULL           0x80000000
#define REG_APE__SMBUS_1_EVENT_ST__MSTR_RX_THRESHOLD_HIT       0x40000000
#define REG_APE__SMBUS_1_EVENT_ST__MSTR_RX_EVENT               0x20000000
#define REG_APE__SMBUS_1_EVENT_ST__MSTR_START_BUSY_0           0x10000000
#define REG_APE__SMBUS_1_EVENT_ST__MSTR_TX_UNDERRUN            0x08000000
#define REG_APE__SMBUS_1_EVENT_ST__SLV_RX_FIFO_FULL            0x04000000
#define REG_APE__SMBUS_1_EVENT_ST__SLV_RX_THRESHOLD_HIT        0x02000000
#define REG_APE__SMBUS_1_EVENT_ST__SLV_RX_EVENT                0x01000000
#define REG_APE__SMBUS_1_EVENT_ST__SLV_START_BUSY_0            0x00800000
#define REG_APE__SMBUS_1_EVENT_ST__SLV_TX_UNDERRUN             0x00400000
#define REG_APE__SMBUS_1_EVENT_ST__SLV_READ_EVENT              0x00200000
#define REG_APE__SMBUS_1_EVENT_ST__SLV_ARP_EVENT               0x00100000

// [APEPER+0x140] SMBUS MSTR WRT
//   "mstr_wrt"
#define REG_APE__SMBUS_1_MSTR_WRT     APE_REG(0x8140)
// [APEPER+0x148] SMBUS SLV WRT
//   "slv_wrt"
#define REG_APE__SMBUS_1_SLV_WRT      APE_REG(0x8148)

// [APEPER+0x180] SMBUS ARP St
//   "arp_st"
#define REG_APE__SMBUS_1_ARP_ST       APE_REG(0x8180)
#define REG_APE__SMBUS_1_ARP_ST__AR0      0x00000002 /* "AR0" */
#define REG_APE__SMBUS_1_ARP_ST__AV0      0x00000001 /* "AV0" */
#define REG_APE__SMBUS_1_ARP_ST__AR1      0x00000020 /* "AR1" */
#define REG_APE__SMBUS_1_ARP_ST__AV1      0x00000010 /* "AV1" */
// unknown, indicates APEPER+0x190 through APEPER+0x19C are valid?
#define REG_APE__SMBUS_1_ARP_ST__UNK100   0x00000100

// [APEPER+0x190] SMBUS UDID 0 WORD3
#define REG_APE__SMBUS_1_UDID_0_WORD3      APE_REG(0x8190)
//  (These are subfields of the standard 128-bit SMBus UDID format.)
#define REG_APE__SMBUS_1_UDID_0_WORD3__CAPAB__MASK       0xFF000000
#define REG_APE__SMBUS_1_UDID_0_WORD3__CAPAB__SHIFT      24
#define REG_APE__SMBUS_1_UDID_0_WORD3__VERSION__MASK     0x00FF0000
#define REG_APE__SMBUS_1_UDID_0_WORD3__VERSION__SHIFT    16
#define REG_APE__SMBUS_1_UDID_0_WORD3__VENDOR_ID__MASK   0x0000FFFF
#define REG_APE__SMBUS_1_UDID_0_WORD3__VENDOR_ID__SHIFT  0

// [APEPER+0x194] SMBUS UDID 0 WORD2
#define REG_APE__SMBUS_1_UDID_0_WORD2      APE_REG(0x8194)
#define REG_APE__SMBUS_1_UDID_0_WORD2__DEVICE_ID__MASK   0xFFFF0000
#define REG_APE__SMBUS_1_UDID_0_WORD2__DEVICE_ID__SHIFT  16
#define REG_APE__SMBUS_1_UDID_0_WORD2__INTERFACE__MASK   0x0000FFFF
#define REG_APE__SMBUS_1_UDID_0_WORD2__INTERFACE__SHIFT  0

// [APEPER+0x198] SMBUS UDID 0 WORD1
#define REG_APE__SMBUS_1_UDID_0_WORD1      APE_REG(0x8198)
#define REG_APE__SMBUS_1_UDID_0_WORD1__SUBSYSTEM_VENDOR_ID__MASK   0xFFFF0000
#define REG_APE__SMBUS_1_UDID_0_WORD1__SUBSYSTEM_VENDOR_ID__SHIFT  16
#define REG_APE__SMBUS_1_UDID_0_WORD1__SUBSYSTEM_DEVICE_ID__MASK   0x0000FFFF
#define REG_APE__SMBUS_1_UDID_0_WORD1__SUBSYSTEM_DEVICE_ID__SHIFT  0

// [APEPER+0x19C] SMBUS UDID 0 WORD0
#define REG_APE__SMBUS_1_UDID_0_WORD0      APE_REG(0x819C)
  // Entire word is VENDOR_SPEC_ID.

// [APEPER+0x1A0] SMBUS UDID 1 WORD3
#define REG_APE__SMBUS_1_UDID_1_WORD3      APE_REG(0x81A0)
//  (These are subfields of the standard 128-bit SMBus UDID format.)
#define REG_APE__SMBUS_1_UDID_1_WORD3__CAPAB__MASK       0xFF000000
#define REG_APE__SMBUS_1_UDID_1_WORD3__CAPAB__SHIFT      24
#define REG_APE__SMBUS_1_UDID_1_WORD3__VERSION__MASK     0x00FF0000
#define REG_APE__SMBUS_1_UDID_1_WORD3__VERSION__SHIFT    16
#define REG_APE__SMBUS_1_UDID_1_WORD3__VENDOR_ID__MASK   0x0000FFFF
#define REG_APE__SMBUS_1_UDID_1_WORD3__VENDOR_ID__SHIFT  0

// [APEPER+0x1A4] SMBUS UDID 1 WORD2
#define REG_APE__SMBUS_1_UDID_1_WORD2      APE_REG(0x81A4)
#define REG_APE__SMBUS_1_UDID_1_WORD2__DEVICE_ID__MASK   0xFFFF0000
#define REG_APE__SMBUS_1_UDID_1_WORD2__DEVICE_ID__SHIFT  16
#define REG_APE__SMBUS_1_UDID_1_WORD2__INTERFACE__MASK   0x0000FFFF
#define REG_APE__SMBUS_1_UDID_1_WORD2__INTERFACE__SHIFT  0

// [APEPER+0x1A8] SMBUS UDID 1 WORD1
#define REG_APE__SMBUS_1_UDID_1_WORD1      APE_REG(0x81A8)
#define REG_APE__SMBUS_1_UDID_1_WORD1__SUBSYSTEM_VENDOR_ID__MASK   0xFFFF0000
#define REG_APE__SMBUS_1_UDID_1_WORD1__SUBSYSTEM_VENDOR_ID__SHIFT  16
#define REG_APE__SMBUS_1_UDID_1_WORD1__SUBSYSTEM_DEVICE_ID__MASK   0x0000FFFF
#define REG_APE__SMBUS_1_UDID_1_WORD1__SUBSYSTEM_DEVICE_ID__SHIFT  0

// [APEPER+0x1AC] SMBUS UDID 1 WORD0
#define REG_APE__SMBUS_1_UDID_1_WORD0      APE_REG(0x81AC)
  // Entire word is VENDOR_SPEC_ID.



// === APE Peripheral Block 0x200 - SMBus Controller 2 =================
// Not present on the 5719.


// === APE Peripheral Block 0x300 - RMU ================================
// RMII MAC Unit. This is probably the R(G?)MII NC-SI interface.

// [APEPER+0x300] BMC->NC RX Status
#define REG_APE__BMC_NC_RX_STATUS     APE_REG(0x8300)
#define REG_APE__BMC_NC_RX_STATUS__PACKET_LEN__MASK 0x07FF0000
// This seems to indicate the SA_HIT field is valid. ?
#define REG_APE__BMC_NC_RX_STATUS__SA_HIT__MASK     0x00003C00
#define REG_APE__BMC_NC_RX_STATUS__FLUSH_DONE       0x00000200
#define REG_APE__BMC_NC_RX_STATUS__FLUSHING         0x00000100
#define REG_APE__BMC_NC_RX_STATUS__IN_PROGRESS      0x00000080
#define REG_APE__BMC_NC_RX_STATUS__EOF              0x00000040
#define REG_APE__BMC_NC_RX_STATUS__UNDERRUN         0x00000020
#define REG_APE__BMC_NC_RX_STATUS__VLAN             0x00000010
#define REG_APE__BMC_NC_RX_STATUS__SA_HIT_VALID     0x00000008
#define REG_APE__BMC_NC_RX_STATUS__PASSTHRU         0x00000004 /* else cmd */
#define REG_APE__BMC_NC_RX_STATUS__BAD              0x00000002
#define REG_APE__BMC_NC_RX_STATUS__NEW              0x00000001

// [APEPER+0x304] BMC->NC RX Source MAC High
//   bit 24 means something, disable?
#define REG_APE__BMC_NC_RX_SRC_MAC_HIGH   APE_REG(0x8304)
// [APEPER+0x308] BMC->NC RX Source MAC Low
#define REG_APE__BMC_NC_RX_SRC_MAC_LOW    APE_REG(0x8308)

// [APEPER+0x30C] BMC->NC RX Source MAC Match n
// For 0 <= n < 8:
//   [APEPER+0x30C+8*n]    MAC High
//   [APEPER+0x30C+8*n+4]  MAC Low
// End: [APEPER+0x354]
#define REG_APE__BMC_NC_RX_SRC_MAC_MATCHN_HIGH(Num) APE_REG(0x830C+8*(Num)+0)
#define REG_APE__BMC_NC_RX_SRC_MAC_MATCHN_LOW(Num)  APE_REG(0x830C+8*(Num)+4)

// [APEPER+0x34C] BMC->NC RX VLAN
//   bits 16-31: VLAN ID probably
#define REG_APE__BMC_NC_RX_VLAN       APE_REG(0x834C)
#define REG_APE__BMC_NC_RX_VLAN__VLAN__MASK   0xFFFF0000
#define REG_APE__BMC_NC_RX_VLAN__VLAN__SHIFT  16

// [APEPER+0x350] BMC->NC
//   Probably for reading from buffer. Read this register repeatedly.

// [APEPER+0x354] BMC->NC RX Control
#define REG_APE__BMC_NC_RX_CONTROL    APE_REG(0x8354)
#define REG_APE__BMC_NC_RX_CONTROL__FLOW_CONTROL         0x01000000
#define REG_APE__BMC_NC_RX_CONTROL__HWM__MASK            0x000007FF
#define REG_APE__BMC_NC_RX_CONTROL__XON_THRESHOLD__MASK  0xFFFFF800
// This field is encoded oddly. If its MSB is set, the XOFF Quanta field
// is 0x7FFF. Otherwise, the XOFF Quanta field is 0xFF|(thisField<<8); the low
// 8 bits are always 0xFF.
#define REG_APE__BMC_NC_RX_CONTROL__XOFF_QUANTA_MASK     0x00FF0000

// [APEPER+0x358] BMC->NC RX Status 1
//   bits 0-31: "rxpkt"
#define REG_APE__BMC_NC_RX_STATUS_1  APE_REG(0x8358)

// [APEPER+0x35C] BMC->NC RX Status 2
// 
#define REG_APE__BMC_NC_RX_STATUS_2  APE_REG(0x835C)
#define REG_APE__BMC_NC_RX_STATUS_2__MAC_CONTROL__MASK 0xFF000000
#define REG_APE__BMC_NC_RX_STATUS_2__TRUNCATED__MASK   0x00FF0000
#define REG_APE__BMC_NC_RX_STATUS_2__DROP__MASK        0x0000FF00
#define REG_APE__BMC_NC_RX_STATUS_2__BAD__MASK         0x000000FF

// [APEPER+0x370] NC->BMC TX Status
#define REG_APE__NC_BMC_TX_STATUS     APE_REG(0x8370)
//   The following are "txFifoEvent" flags.
#define REG_APE__NC_BMC_TX_STATUS__IN_FIFO__MASK    0x07FF0000
#define REG_APE__NC_BMC_TX_STATUS__UNDERRUN         0x00000001
#define REG_APE__NC_BMC_TX_STATUS__HIT_LWM          0x00000002
#define REG_APE__NC_BMC_TX_STATUS__EMPTY            0x00000004
#define REG_APE__NC_BMC_TX_STATUS__FULL             0x00000008
#define REG_APE__NC_BMC_TX_STATUS__LAST_COUNT_FULL  0x00000010

// [APEPER+0x374] NC->BMC TX Control
#define REG_APE__NC_BMC_TX_CONTROL   APE_REG(0x8374)
#define REG_APE__NC_BMC_TX_CONTROL__LWM__MASK      0x07FF0000
#define REG_APE__NC_BMC_TX_CONTROL__LAST_BYTE_COUNT__MASK  0x00001800
#define REG_APE__NC_BMC_TX_CONTROL__STORE_FORWARD  0x00000100 /* "storeFwd", else "cutThru" */
#define REG_APE__NC_BMC_TX_CONTROL__POISON         0x00000200
#define REG_APE__NC_BMC_TX_CONTROL__XOFF           0x00000400
#define REG_APE__NC_BMC_TX_CONTROL__NOT_TX_FIFO_MASK__LAST_COUNT_FULL 0x00000010 /* "!txFifoMsk: lastCntFull" */
#define REG_APE__NC_BMC_TX_CONTROL__NOT_TX_FIFO_MASK__FULL            0x00000008
#define REG_APE__NC_BMC_TX_CONTROL__NOT_TX_FIFO_MASK__EMPTY           0x00000004
#define REG_APE__NC_BMC_TX_CONTROL__NOT_TX_FIFO_MASK__HIT_LWM         0x00000002
#define REG_APE__NC_BMC_TX_CONTROL__NOT_TX_FIFO_MASK__UNDERRUN        0x00000001

// unknown: 0x378
//   lots of words are written to this at once?
//   It seems like you write a packet to the RMU by writing the words to this
//   in sequence. Appears to be to the BMC.
//
// unknown: 0x37C
//   This seems to be masked by LAST_BYTE_COUNT__MASK to allow a frame to have
//   a size which is not a multiple of four bytes. It also seems to indicate
//   the end of a frame (for frames with sizes which are a multiple of four
//   bytes, LAST_BYTE_COUNT__MASK is set to zero and this is written as zero).

// [APEPER+0x380] NC->BMC TX Status 1
// bits 0-31: "txpkt"
#define REG_APE__NC_BMC_TX_STATUS_1  APE_REG(0x8380)

// [APEPER+0x3A0] RMU Control
// Most of these are enable bits.
#define REG_APE__RMU_CONTROL           APE_REG(0x83A0)
#define REG_APE__RMU_CONTROL__IFG__MASK  0x0000001F
#define REG_APE__RMU_CONTROL__RX         0x00000040
#define REG_APE__RMU_CONTROL__TX         0x00000020
#define REG_APE__RMU_CONTROL__LPBK       0x00000010
#define REG_APE__RMU_CONTROL__TX_DRV     0x00000008
#define REG_APE__RMU_CONTROL__AUTO_DRV   0x00000004
#define REG_APE__RMU_CONTROL__RST_RX     0x00000002
#define REG_APE__RMU_CONTROL__RST_TX     0x00000001

// [APEPER+0x3A4] Arb Control
// Most of these are "arb" bits.
// To enable, set DISABLE=0, AUTO_BY=0, BYPASS=0, START=1, PACKAGE_ID=...
//   (CHANNELN_INFO__HWARB set)
// To disable, set DISABLE=0, AUTO_BY=1, BYPASS=0, START=0 (?)
//   (CHANNELN_INFo__HWARB cleared)
#define REG_APE__ARB_CONTROL          APE_REG(0x83A4)
#define REG_APE__ARB_CONTROL__PACKAGE_ID__MASK  0x00000007
#define REG_APE__ARB_CONTROL__TO__MASK          0xFFFF0000
#define REG_APE__ARB_CONTROL__TKNREL__MASK      0x00001F00
#define REG_APE__ARB_CONTROL__XOFF_DIS  0x00000080
#define REG_APE__ARB_CONTROL__AUTO_BY   0x00000040
#define REG_APE__ARB_CONTROL__BYPASS    0x00000020
#define REG_APE__ARB_CONTROL__START     0x00000010
#define REG_APE__ARB_CONTROL__DISABLE   0x00000008

// [APEPER+0x3B0] ???
// [APEPER+0x3B4] ???

// === APE Peripheral Block 0x400 - Locks ==============================
// tg3 suggests the lock ports below are allocated as follows:
//   Port 0: PHY0
//   Port 1: GRC
//   Port 2: PHY1
//   Port 3: PHY2
//   Port 4: MEM
//   Port 5: PHY3
//   Port 6: Unused?
//   Port 7: GPIO
//
// Seems to be confirmed by APE port numbers for APE functions in bootcode.

// [APEPER+0x400]
#define REG_APE__PER_LOCK_REQ           APE_REG(0x8400) /* +(func<<2) */
// 0x0000_0010: Used by bootcode to request lock
// For the PHY ports (or always for function 0), driver uses
//   0x0000_1000: TG3_APE_PER_LOCK_REQ: APELOCK_PER_REQ_DRIVER
// For non-PHY ports on non-zero functions, driver uses
//   0x0000_0002: Function 1
//   0x0000_0004: Function 2
//   0x0000_0008: Function 3
#define REG_APE__PER_LOCK_REQ__APE      0x00000001 /* used by APE */
#define REG_APE__PER_LOCK_REQ__BOOTCODE 0x00000010 /* used by bootcode */
#define REG_APE__PER_LOCK_REQ__DIAG     0x00000020 /* used by diag, check this again */
#define REG_APE__PER_LOCK_REQ__DRIVER   0x00001000 /* tg3 */
#define REG_APE__PER_LOCK_REQ_PHY0     APE_REG(0x8400) /* port 0: PHY0 */
#define REG_APE__PER_LOCK_REQ_GRC      APE_REG(0x8404) /* port 1: GRC  */
#define REG_APE__PER_LOCK_REQ_PHY1     APE_REG(0x8408) /* port 2: PHY1 */
#define REG_APE__PER_LOCK_REQ_PHY2     APE_REG(0x840C) /* port 3: PHY2 */
#define REG_APE__PER_LOCK_REQ_MEM      APE_REG(0x8410) /* port 4: MEM  */
#define REG_APE__PER_LOCK_REQ_PHY3     APE_REG(0x8414) /* port 5: PHY3 */
#define REG_APE__PER_LOCK_REQ_PORT6    APE_REG(0x8418) /* port 6 */
#define REG_APE__PER_LOCK_REQ_GPIO     APE_REG(0x841C) /* port 7: GPIO */

#define APE_PORT_PHY0  0
#define APE_PORT_GRC   1
#define APE_PORT_PHY1  2
#define APE_PORT_PHY2  3
#define APE_PORT_MEM   4
#define APE_PORT_PHY3  5
/* Port 6: unused? */
#define APE_PORT_GPIO  7

// [APEPER+0x420]
#define REG_APE__PER_LOCK_GRANT         APE_REG(0x8420) /* +(func<<2) */
#define REG_APE__PER_LOCK_GRANT__APE      0x00000001 /* used by APE */
#define REG_APE__PER_LOCK_GRANT__BOOTCODE 0x00000010 /* used by bootcode */
#define REG_APE__PER_LOCK_GRANT__DRIVER   0x00001000 /* tg3 */
#define REG_APE__PER_LOCK_GRANT_PHY0   APE_REG(0x8420)
#define REG_APE__PER_LOCK_GRANT_GRC    APE_REG(0x8424)
#define REG_APE__PER_LOCK_GRANT_PHY1   APE_REG(0x8428)
#define REG_APE__PER_LOCK_GRANT_PHY2   APE_REG(0x842C)
#define REG_APE__PER_LOCK_GRANT_MEM    APE_REG(0x8430)
#define REG_APE__PER_LOCK_GRANT_PHY3   APE_REG(0x8434)
#define REG_APE__PER_LOCK_GRANT_PORT6  APE_REG(0x8438)
#define REG_APE__PER_LOCK_GRANT_GPIO   APE_REG(0x843C)
// End: 0x8440

// ============ End of APE Periperal Blocks ============

/*
from diag:
  32 16  8
   !  S  X  Configuration space (32)
   #        Registers
   *        SRAM
   &        SRAM (ASCII)
      m     MII Registers
   ~        VPD Access
   g        APE Registers                   APERegRd
   r        APE Shared Memory               APEShrMemRd
   h        APE Scratchpad Memory
              N.B. This is -not- the 0xC003_0000 region (unless it is aliased there),
              it is accessed indirectly via the 0xF8/0xFC method, needs more
              research
   p        APE Peripheral                  APEPeripheralRd
   u        APE UART                        ...
   M        5717 APE mutex registers
   y        Physical Memory Access
   o        OTP Registers

   $        NVM?
 */

#ifndef OTG_APE
// Gets the device function number. This is the PCI function number, which
// indicates which port we are (each port is a separate function and has a
// separate CPU; we query our own PCI MMIO space's STATUS register's function
// number bits to figure out which we are.)
static inline uint8_t GetFunctionNo(void) {
  return (GetImmutableReg(REG_STATUS) >> REG_STATUS__FUNC_NUMBER__SHIFT) & PORT_MASK;
}
static inline uint8_t GetFunctionNo_Dynamic(void) {
  return (GetReg(REG_STATUS) >> REG_STATUS__FUNC_NUMBER__SHIFT) & PORT_MASK;
}

// Which media mode have we been configured to use?
// 1 if SERDES, 0 if copper.
static inline uint8_t GetMediaMode(void) {
  return !!(GetImmutableReg(REG_SGMII_STATUS) & REG_SGMII_STATUS__MEDIA_SELECTION_MODE);
}

static inline bool IsGPHY(void) { // is this a copper port?
  return !GetMediaMode();
}

static inline bool IsSERDES(void) { // is this an SFP port?
  return GetMediaMode();
}

// Gets the PHY number.
//
// BCM5719
//           Port 0    Port 1    Port 2    Port 3
//   GPHY    0x01      0x02      0x03      0x04
//   SERDES  0x08      0x09      0x0A      0x0B
//
// BCM firmware used a table to look these up, but who needs that?
static inline uint8_t GetPHYNo(void) {
  return (GetMediaMode()<<3) | (GetFunctionNo()+IsGPHY());
}

static const uint8_t g_apePortTable[];

static inline uint8_t GetPHYNo_A(void) {
  return g_apePortTable[GetFunctionNo()];
}

// Gets the GPHY or SERDES given the function number, ignoring the current
// value of REG_SGMII_STATUS__MEDIA_SELECTION_MODE.
static inline uint8_t GetGPHYNo(void) {
  return GetFunctionNo()+1;
}
static inline uint8_t GetSERDESNo(void) {
  return (1<<3)|GetFunctionNo();
}
#endif

static inline uint32_t SwapEndian32(uint32_t x) {
  return __builtin_bswap32(x); // GCC
}
static inline uint32_t SwapHalves32(uint32_t x) {
  return (x >> 16) | (x << 16);
}


/* ---------------------------------------------------------------- */
// Common functions.

enum {
  ARB_ACQUIRE = 0x0080,
  ARB_RELEASE = 0x0100,
};


/* ---------------------------------------------------------------- */
// Memory map.

#define S1_ENTRYPOINT 0x08003800
#define S2_ENTRYPOINT 0x08000000


/* ---------------------------------------------------------------- */
// Flash format.

// The full layout is as follows:
//   MAIN HEADER (the below structure, 0x28C bytes in size)
//   STAGE1 CODE (entrypoint at start)  @0x28C
//   STAGE1 DATA
//   STAGE1 CRC WORD                    @(stage1Offset+stage1Size*4-4)
//   -- STAGE1 END --                   @(stage1Offset+stage1Size*4)
//
//   STAGE2 HEADER                      @(stage1Offset+stage1Size*4)
//   STAGE2 CODE (entrypoint at start)  @(stage1Offset+stage1Size*4+8)
//   STAGE2 DATA
//   STAGE2 CRC WORD                    @(stage1Offset+stage1Size*4+8+stage2Size-4)
//   -- STAGE2 END --                   @(stage1Offset+stage1Size*4+8+stage2Size)
//
//   NCSI HEADER/IMAGE (compressed, unknown format)   @getTag(0x0D).v1
//   -- NCSI END --                                   @getTag(0x0D).v1 + getTag(0x0D).lower24*4
//
//   NOR PADDING TO DEVICE SIZE (0xFF)

// The header contains some tags, which are fixed-length type-value
// structures used to point to data. See enum below for type-specific
// information.
/*typedef struct __attribute__((packed)) {
  uint32_t header; // Upper 8 bits: type
                   // Lower 24 bits: arbitrary, but often size in words. A tag
                   // is ignored (considered invalid) if the low 22 bits are
                   // zero. Most of the tags are {0,0,0} and thus unallocated.
                   // When searching for a tag, each fixed-length tag in the
                   // tag array is considered until a tag is found with
                   // matching type field and nonzero lower 22 bits.
  uint32_t v1;     // Type-specific meaning; v1 is often an offset.
  uint32_t v2;     // Type-specific meaning; often zero and unused.
} otg_header_tag;
static_assert(sizeof(otg_header_tag) == 12, "otg_header_tag");*/

typedef struct __attribute__((packed)) {
  uint32_t loadAddr;
    // Often not used/ignored.
  uint32_t typeSize; 
    // tttt tttt  XXss ssss  ssss ssss  ssss ssss
    // t: Image type. 8 bits.
    // X: Extra bits, sometimes used for two flags?
    // s: Image size in words. 22 bits.
  uint32_t offset; // in bytes.
} otg_directory_entry;
static_assert(sizeof(otg_directory_entry) == 12, "otg_directory_entry");

enum {
  // NOTE: Tag types with the MSB set (0x80-0xFF) MUST be located in an extended
  // directory and not a normal directory. Likewise, tag types without the MSB
  // set should not be located in an extended directory.

  // Standard Directory Types
  // ------------------------

  // Expansion ROM Pointer. Offset of Expansion ROM is in tag.v1, size probably
  // in header low 24 bits. v2 probably unused.
  OTG_HEADER_TAG_TYPE__PXE           = 0x00<<24,

  OTG_HEADER_TAG_TYPE__ASF_INIT      = 0x01<<24,
  OTG_HEADER_TAG_TYPE__ASF_CPUA      = 0x02<<24,
  OTG_HEADER_TAG_TYPE__ASF_CPUB      = 0x03<<24,
  OTG_HEADER_TAG_TYPE__ASF_CFG       = 0x04<<24,
  OTG_HEADER_TAG_TYPE__ISCSI_CFG     = 0x05<<24, // iSCSI configuration for function 0
  OTG_HEADER_TAG_TYPE__ISCSI_CFG_PRG = 0x06<<24, // iSCSI configuration program
  OTG_HEADER_TAG_TYPE__USER_BLOCK    = 0x07<<24,
  OTG_HEADER_TAG_TYPE__BRSF_BLOCK    = 0x08<<24,
  OTG_HEADER_TAG_TYPE__ISCSI_BOOT    = 0x09<<24, // iSCSI boot program
  OTG_HEADER_TAG_TYPE__ASF_MBOX      = 0x0A<<24,
  OTG_HEADER_TAG_TYPE__ISCSI_CFG_1   = 0x0B<<24, // iSCSI configuration for function 1
  OTG_HEADER_TAG_TYPE__APE_CFG       = 0x0C<<24,

  // Pointer to NCSI (that is, APE) executable image.
  // v1 is the offset in bytes in flash, header low24 is the size in
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

  // Extended Directory Types
  // ------------------------

  OTG_HEADER_TAG_TYPE__88            = 0x88<<24, // probably CCM code
  OTG_HEADER_TAG_TYPE__ISCSI_CFG_2   = 0x82<<24, // iSCSI configuration for function 2
  OTG_HEADER_TAG_TYPE__ISCSI_CFG_3   = 0x83<<24, // iSCSI configuration for function 3

  //
  OTG_HEADER_TAG_TYPE_MASK                   = 0xFF000000,
};

// This header comes at the start of flash and is at the start of the flash
// image that gets written to flash. It contains some fields which can be
// modified in the field to customize the image (e.g. MAC addresses).
#define PP_CAT_(X,Y) X##Y
#define PP_CAT(X,Y) PP_CAT_(X,Y)
#define UNKNOWN() PP_CAT(_unk_,__COUNTER__)
typedef struct __attribute__((packed)) {
                                //1=known to be read by stage1 code
                                //----------------------------------------------------------------------
  uint32_t magic;               //   [ 00] HEADER_MAGIC (0x669955AA)
  uint32_t s1Entrypoint;        //1  [ 04] Stage1 entrypoint address in RAM.
  uint32_t s1Size;              //1  [ 08] Stage1 size IN WORDS. Slightly off?
  uint32_t s1Offset;            //1  [ 0C] Offset to start of stage1 code in flash.
  uint32_t bootHdrCRC;          //   [ 10] 0x352333D4 -- CRC up to this point


  otg_directory_entry dir[8];   //1  [ 14]

  // BEGIN MANUFACTURING SECTION
  uint8_t mfrFormatRev;         //   [ 74] 44
  uint8_t dirCRC;               //   [ 75] 02
  uint16_t mfrLen;              //   [ 76] 00 8c ] Manufacturing Section Length (including mfrFormatRev/dirCRC/mfrLen)

  uint32_t reserved078;         //   [ 78] 0     ] Reserved/0 according to BCM headers.
                                //               ] Previously "PHY ID of physical device".

  uint32_t mac0[2];             //1  [ 7C] Upper 16 bits are zero/unused.

  char partNo[16];              //   [ 84] "BCM95719", zero padded

  char partRev[2];              //   [ 94] 41 30  ] Hardware revision. e.g. 'A0'
  uint16_t fwRev;               //1  [ 96] 01 2B  ] major byte/minor byte
  char     mfrDate[4];          //1  [ 98] 0      ] Manufacturing date, 4 ASCII bytes, "wwyy".
  uint16_t func0PXEVLAN;        //   [ 9C] 00 00
  uint16_t func1PXEVLAN;        //   [ 9E] 00 00

  uint16_t pciDevice;           //1  [ A0] 0x1657 BCM5719
  uint16_t pciVendor;           //1  [ A2] 0x14E4 Broadcom
  uint16_t pciSubsystem;        //1  [ A4] 0x1657 BCM5719     // Unused...
  uint16_t pciSubsystemVendor;  //1  [ A6] 0x14E4 Broadcom

  uint16_t cpuClock;            //   [ A8] 00 42          ] In MHz. 66 MHz. Legacy from PCI?
                                //           Used to be 2 bytes but seems to be two fields
                                //           now, one of which appears to be SMBus-related.
  uint8_t  ncSMBUSAddr;         //   [ AA] 0              ] Network Controller SMBus Address
                                //                        ] (Talos: 0, Expansion card: 0x64)
  uint8_t  bmcSMBUSAddr;        //   [ AB] 0              ] BMC SMBus Address (only 0 observed)
  uint32_t backupMAC0[2];       //   [ AC] 0            ] Backup MACs. Apparently was "software keys"
  uint32_t backupMAC1[2];       //   [ B4] 0            ] in the past for feature enable.
  uint8_t  powerDissipated[4];  //1  [ BC] 0A 00 00 64. => Reg 0x6414. Entry 0: Power dissipated in D3, 1: D2, 2: D1, 3: D0.
  uint8_t  powerConsumed[4];    //1  [ C0] 0A 00 00 64. => Reg 0x6410. Entry 0: Power dissipated in D3, 1: D2, 2: D1, 3: D0.
  uint32_t func0CfgFeature;     //1  [ C4] C5 C0 00 80 - Function 0 GEN_CFG_FEATURE.  FEATURE CONFIG
  uint32_t func0CfgHW;          //1  [ C8] 00 00 40 14 - Function 0 GEN_CFG_HW.       HW CONFIG

  uint32_t mac1[2];             //1  [ CC] Upper 16 bits are zero/unused.

  uint32_t func1CfgFeature;     //1  [ D4] C5 C0 00 00 - Function 1 GEN_CFG_FEATURE.  FEATURE CONFIG
  uint32_t func1CfgHW;          //1  [ D8] 00 00 40 14 - Function 1 GEN_CFG_HW.       HW CONFIG
  uint32_t cfgShared;           //1  [ DC] 00 C2 AA 38 - GEN_CFG_SHARED.              SHARED CONFIG
  uint32_t powerBudget0;        //1  [ E0] 2C 16 3C 2C ]
  uint32_t powerBudget1;        //1  [ E4] 00 00 23 0A ]|
  uint32_t serworksUse;         //   [ E8] 00 00 00 00<-|-- Unknown purpose.
  uint16_t func0SERDESOverride; //   [ EC] 00 00 <-|    |-- S1MegaUltraInit (22)
  uint16_t func1SERDESOverride; //   [ EE] 00 00 <-|----|--------- Unknown purpose, probably vestigial.
  uint16_t tpmNVMSize;          //   [ F0] 00 00 <=|====|========= Unknown purpose, probably vestigial.
  uint16_t macNVMSize;          //   [ F2] 00 00 <=|    | Once expressed n where the size was 2**n KiB. 0=unknown.
  uint32_t powerBudget2;        //1  [ F4] 00 00 00 00 ]|
  uint32_t powerBudget3;        //1  [ F8] 00 00 00 00 ]
  uint32_t mfrCRC;              //   [ FC] 00 30 65 D9
  // END MANUFACTURING SECTION

  uint8_t vpd[256];             // 2 [100] PCI-format VPD data. Rewriting this is probably allowed.
                                //         Has its own checksum, as specified by the VPD format.

  // BEGIN MANUFACTURING SECTION 2
  uint16_t mfr2Unk;             //   [200] 00 00          -- Unknown, probably unused.
  uint16_t mfr2Len;             //   [202] 00 8C          -- Length of manufacturing section 2.
  uint32_t UNKNOWN();           //   [204] 00 00 00 00    -- Could be reserved.

  uint32_t mac2[2];             //1  [208] Upper 16 bits are zero/unused.

  uint32_t UNKNOWN();           //   [210] 0
  uint32_t UNKNOWN();           //   [214] 0
  uint32_t UNKNOWN();           //   [218] 0
  uint32_t cfg5;                //1  [21C] 0   - GEN_CFG_5. g_unknownInitWord3
  uint32_t UNKNOWN();           //   [220] 0
  uint32_t UNKNOWN();           //   [224] 0
  uint32_t UNKNOWN();           //   [228] 0

  uint16_t pciSubsystemF1GPHY;  //1  [22C] 19 81 ] PCI Subsystem.
  uint16_t pciSubsystemF0GPHY;  //1  [22E] 19 81 ] These are selected based on the
  uint16_t pciSubsystemF2GPHY;  //1  [230] 19 81 ] function number and whether the NIC is a
  uint16_t pciSubsystemF3GPHY;  //1  [232] 19 81 ] GPHY (copper) or SERDES (SFP) NIC.
  uint16_t pciSubsystemF1SERDES;//1  [234] 16 57 ] BCM5719(?). Probably not programmed correctly
  uint16_t pciSubsystemF0SERDES;//1  [236] 16 57 ] since Talos II doesn't use SERDES.
  uint16_t pciSubsystemF3SERDES;//1  [238] 16 57 ]
  uint16_t pciSubsystemF2SERDES;//1  [23A] 16 57 ]

  uint32_t UNKNOWN();           //   [23C] 0
  uint32_t UNKNOWN();           //   [240] 0
  uint32_t UNKNOWN();           //   [244] 0
  uint32_t UNKNOWN();           //   [248] 0
  uint32_t UNKNOWN();           //   [24C] 0
  uint32_t func2CfgFeature;     //1  [250] C5 C0 00 00 - Function 2 GEN_CFG_1E4.
  uint32_t func2CfgHW;          //1  [254] 00 00 40 14 - Function 2 GEN_CFG_2.

  uint32_t mac3[2];             //1  [258] Upper 16 bits are zero/unused.

  uint32_t func3CfgFeature;     //1  [260] C5 C0 00 00 - Function 3 GEN_CFG_1E4.
  uint32_t func3CfgHW;          //1  [264] 00 00 40 14 - Function 3 GEN_CFG_2.
  uint32_t UNKNOWN();           //   [268] 0
  uint32_t UNKNOWN();           //   [26C] 0
  uint32_t UNKNOWN();           //   [270] 0
  uint32_t UNKNOWN();           //   [274] 0
  uint32_t func0CfgHW2;         //1  [278] 00 00 00 40 - Function 0 GEN_CFG_2A8.
  uint32_t func1CfgHW2;         //1  [27C] 00 00 00 40 - Function 1 GEN_CFG_2A8.
  uint32_t func2CfgHW2;         //1  [280] 00 00 00 40 - Function 2 GEN_CFG_2A8.
  uint32_t func3CfgHW2;         //1  [284] 00 00 00 40 - Function 3 GEN_CFG_2A8.
  uint32_t mfr2CRC;             //   [288] 1A AC 41 A6 // could be CRC
  // END MANUFACTURING SECTION 2
} otg_header;
static_assert(sizeof(otg_header) == 0x28C, "otg header size");
#undef UNKNOWN

typedef struct __attribute__((packed)) {
  uint32_t magic;       // HEADER_MAGIC
  uint32_t s2Size;      // Size of the stage2 in bytes.
  // Entrypoint code follows immediately.
} otg_s2header;

#define APE_SECTION_FLAG_COMPRESSED         (1U<<24)
#define APE_SECTION_FLAG_CHECKSUM_IS_CRC32  (1U<<25)
#define APE_SECTION_FLAG_ZERO_ON_FAST_BOOT  (1U<<28)

typedef struct {
  uint32_t loadAddr;
  uint32_t offsetFlags;
  uint32_t uncompressedSize;
  uint32_t compressedSize;
  uint32_t checksum;
} ape_section;
static_assert(sizeof(ape_section) == 0x14, "ape section descriptor size");

typedef struct __attribute__((packed)) {
  char magic[4];           // [000] "BCM\x1A", or "BUB\x1A" for diag image
  uint32_t unk04;          // [004] 0x03070700
  char imageName[16];      // [008] e.g. "NCSI 1.3.7"
  uint32_t imageVersion;   // [018] 0xMMmmpp00, vMM.mm.pp
  uint32_t entrypoint;     // [01C] e.g. 0x0010_80C1
  uint8_t  unk020;         // [020] 0x00
  uint8_t  headerSize;     // [021] Header size in words. Always 0x1E.
  uint8_t  unk022;         // [022] 0x04
  uint8_t  numSections;    // [023] The number of sections. Always 4.
  uint32_t headerChecksum; // [024] Ethernet CRC32 over header, with this field zero.

  ape_section sections[4]; // [028]
} ape_header;
static_assert(sizeof(ape_header) == 0x78, "ape header size");
