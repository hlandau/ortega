OUTPUT_FORMAT(binary)
ENTRY(AEntrypoint)

SECTIONS {
  . = 0x00100C00 - 0x78;

  .header . : ALIGN(4) SUBALIGN(4) {
    *(.header)
  }

  ASSERT(. == 0x00100C00, "header size")
  ASSERT(SIZEOF(.header) == 0x78, "header size")

  .scratchtext . : ALIGN(4) SUBALIGN(4) {
    *(.scratchtext)
    . = ALIGN(4);
  }

  _ScratchTextSize = SIZEOF(.scratchtext);
  _ScratchTextStart  = ADDR(.scratchtext);
  _ScratchTextEnd    = _ScratchTextStart + SIZEOF(.scratchtext);
  _ScratchTextOffset = _ScratchTextStart - ADDR(.header);

  . = 0x00108000;

  .text . : ALIGN(4) SUBALIGN(4) {
    *(.isrtable)
    *(.textstart)
    *(.text)
    . = ALIGN(4);
  }

  _TextSize   = SIZEOF(.text);
  _TextStart  = ADDR(.text);
  _TextEnd    = _TextStart + SIZEOF(.text);
  _TextOffset = _ScratchTextOffset + SIZEOF(.scratchtext);

  ASSERT(_TextStart == 0x00108000, "textstart")

  .data . : ALIGN(4) SUBALIGN(4) {
    *(.data)
    *(.rodata)
    *(.rodata.*)

    . = ALIGN(4);
    /* This is the trailing CRC placeholder. We pull a trick here and stick it
     * at the end of .data. The idea is that .bss shouldn't take up any space
     * in the file; so this will come at the end of the file and apestamp will
     * be happy. If something changes that causes bss to start emitting data,
     * the build will break. So this forms an additional sanity check.
     */
    *(.trailingcrc)
  }

  _DataSize   = SIZEOF(.data);
  _DataStart  = ADDR(.data);
  _DataEnd    = _DataStart + SIZEOF(.data);
  _DataOffset = _TextOffset + SIZEOF(.text);

  .bss . : ALIGN(4) SUBALIGN(4) {
    *(.bss)
    . = ALIGN(4);
  }

  _BSSSize   = SIZEOF(.bss);
  _BSSStart  = ADDR(.bss);
  _BSSEnd    = _BSSStart + SIZEOF(.bss);
  _BSSOffset = _DataOffset + SIZEOF(.data);

  _StackEnd  = 0x118000;

  ASSERT((_StackEnd - _BSSEnd) >= 0x2000, "minimum stack room")

  /DISCARD/ : {
    *(.comment)
    *(.note.GNU-stack)
    *(.eh_frame)
    *(.got)
    *(.reginfo)
    *(.mdebug.abi32)
    *(.pdr)
    *(.ARM.exidx*)
    *(.ARM.attributes)
  }

  _ScratchTextOffsetFlags = _ScratchTextOffset | (1 << 25) | (1 << 26) | (1 << 27); /* CRC32, BIT26, BIT27 */
  _TextOffsetFlags        = _TextOffset        | (1 << 25) | (1 << 26) | (1 << 27); /* CRC32, BIT26, BIT27 */
  _DataOffsetFlags        = _DataOffset        | (1 << 25); /* CRC32 */
  _BSSOffsetFlags         = _BSSOffset         | (1 << 28); /* ZERO_ON_FAST_BOOT */

  _AEntrypointM         = AEntrypoint & 0xFFFFFFFE;
  ASSERT(_AEntrypointM == (0x001080C0), "entrypoint")
}
