OUTPUT_FORMAT(binary)
ENTRY(APEShellEntrypoint)

SECTIONS {
  . = 0x00100000;

  .text . : ALIGN(4) SUBALIGN(4) {
    *(.textstart)
    *(.text)
    *(.data)
    *(.bss)
    *(.rodata)
    *(.rodata.*)
    *(.trailer)
  }

  /DISCARD/ : {
    *(.comment)
    *(.note.GNU-stack)
    *(.eh_frame)
    *(.got)
    *(.reginfo)
    *(.mdebug.abi32)
    *(.pdr)
    *(.ARM.exidx*)
  }

  ASSERT(ADDR(.text)        == (0x00100000), "textstart")
  /*ASSERT(APEShellEntrypoint == (0x00100000), "addr 1")*/
}
