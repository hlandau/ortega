OUTPUT_FORMAT(binary)
ENTRY(S2Entrypoint)

SECTIONS {
  . = 0x08000000 - 8;
  .header . : {
    *(.header)
    *(.textstart)
  }

  ASSERT(ADDR(.header)   == 0x07FFFFF8, "header VMA")
  ASSERT(SIZEOF(.header) == 0x00000020, "header size")

  . = 0x08000018;
  .text . : ALIGN(4) SUBALIGN(4) {
    *(.textasm)
    *(.text)
    *(.data)
    *(.bss)
    *(.rodata)
    *(.rodata.*)
  }

  .crc : ALIGN(4) SUBALIGN(4) {
    *(.crc)
  }

  ASSERT(S2Entrypoint    == 0x08000000, "S2Entrypoint")
  ASSERT(S2Entrypoint2   == 0x08000018, "S2Entrypoint2")
  ASSERT(SIZEOF(.crc)    == 4,          "CRC size")

  S2SizeInBytes = ADDR(.crc) + SIZEOF(.crc) - 0x08000000;
  S2StackTop = 0x08007000;

  /* lld doesn't support --orphan-sections=discard, ugh. */
  /DISCARD/ : {
    *(.comment)
    *(.note.GNU-stack)
    *(.eh_frame)
    *(.MIPS.abiflags)
    *(.reginfo)
    *(.got)
    *(.mdebug.abi32)
    *(.pdr)
  }
}
