OUTPUT_FORMAT(binary)
ENTRY(S1Entrypoint)

SECTIONS {
  . = 0x08003800 - 0x28c;

  .header . : ALIGN(4) SUBALIGN(4) {
    *(.header)
    S1TextStartBegin = .;
    *(.textstart)
    S1TextStartEnd = .;
  }

  ASSERT(S1TextStartBegin == (0x08003800), "textstart")
  ASSERT(S1TextStartEnd   == (0x08003800 + 12), "textstart end")
  ASSERT(ADDR(.header)    == (0x08003800 - 0x28c), "header VMA")
  ASSERT(SIZEOF(.header)  == (0x28C + 12), "header size")
  ASSERT(.                == (0x08003800 + 12), "cur")

  .text . : ALIGN(4) SUBALIGN(4) {
    *(.textasm)
    *(.text)
    *(.data)
    *(.bss)
    *(.rodata)
    *(.rodata.*)
  }

  /*S1SizeInWords_ = ((ADDR(.text) + SIZEOF(.text) - ADDR(.text)) + 4 + 3) / 4;*/

  S1StackTop = 0x08007000;

  S1TextOffset = S1TextStartBegin - gS1_otgHeader;

  .stage2 . : ALIGN(4) SUBALIGN(4) {
    *(.stage2)
  }

  /* Add 4 here because we put the S1 CRC word at the start of the S2 section. */
  S1SizeInWords = ((ADDR(.stage2) - S1TextStartBegin + 4) + 3) / 4;
  _qS1SizeInWords = (ADDR(.stage2) + 4 /* CRC word */ - ADDR(.text) + 3) / 4;

  .ape . : ALIGN(4) SUBALIGN(4) {
    *(.ape)
  }

  APEImageStartPhys = APEImageStart - 0x08003570;
  APEImageSizeInWords = (((APEImageEnd - APEImageStart) / 4));
  APEImageSizeInWordsWithTypeByte = APEImageSizeInWords | 0x0D000000;

  /DISCARD/ : {
    *(.comment)
    *(.note.GNU-stack)
    *(.eh_frame)
    *(.got)
    *(.MIPS.abiflags)
    *(.reginfo)
    *(.mdebug.abi32)
    *(.pdr)
  }
}
