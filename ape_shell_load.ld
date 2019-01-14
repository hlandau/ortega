OUTPUT_FORMAT(binary)
ENTRY(APEShellEntrypoint)

SECTIONS {
  . = 0x00100A00 - 0x78;

  .header . : ALIGN(4) SUBALIGN(4) {
    *(.header)
  }

  .text . : ALIGN(4) SUBALIGN(4) {
    *(.textstart)
    *(.text)
    *(.data)
    *(.bss)
    *(.rodata)
    *(.rodata.*)
    *(.trailer)
    . = ALIGN(4);
  }

  _TextSize   = SIZEOF(.text);
  _TextStart  = ADDR(.text);
  _TextEnd    = _TextStart + SIZEOF(.text);

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

  ASSERT(ADDR(.text)        == (0x00100A00), "textstart")
}
