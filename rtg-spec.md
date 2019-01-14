# RTG Specification

## Prologue

This specification describes the technical requirements for boot firmware
for BCM5719 network controller chips.

### Background Reading

It is advised that you familiarise yourself with the [register manual for the
BCM5719]() published by Broadcom before reading this specification.

### Introduction

The BCM5719 network controller chip is a member of the BCM5717 family of
Broadcom Gigabit Ethernet network controller chips, which includes the BCM5717,
BCM5718, BCM5719 and BCM5720. The BCM5717, BCM5718 and BCM5720 have two Gigabit
Ethernet ports, while the BCM5719 has four. Although the chips have only very
minor variations between them, this specification focuses solely on the
BCM5719.

The BCM5719 is attached to a small nonvolatile memory device (hereafter
referred to as NVM). A variety of devices are supported; a typical example is a
512KiB Atmel 45DB041E.

This NVM device is used to store many things:

  - Configuration information (including MAC addresses)
  - Bootcode firmware
  - APE firmware
  - For x86 applications, also present will be:
    - a PCI Option ROM image (typically both an x86 PC BIOS version and an
      x86-64 UEFI BIOS version), which implements PXE and which can also
      chainload the following programs;
    - a configuration management tool which can be chainloaded by the Option
      ROM;
    - an iSCSI boot program which can be chainloaded by the Option ROM.
  - Vital Product Data, such as device serial numbers, asset tags, etc.

If you have read the BCM5719 register manual, you'll note that there are two
boot modes: Legacy, which boots from NVM as described above, and Self-Boot,
which boots from a built-in ROM and which requires only a very small NVM device
to hold MAC addresses, etc. Although this makes it sound like Legacy boot is
deprecated, it's clear from research that this is far from the case, and most
applications of the BCM5719 probably use “legacy” boot, as it appears to be
necessary to much of the advanced functionality of the chip, like the APE and
option ROMs.

**Option ROMs.** The PCI Option ROMs are not integral to the function of the
device, and in any case are completely irrelevant in applications of the device
to non-x86 platforms, so we don't discuss them further.

**Bootcode.** The “bootcode” is a small amount (~15KiB) of code which runs on
the RX CPUs. Each function (port) of the device has an “RX RISC” CPU. This is a
big-endian MIPS core which loads the bootcode from NVM and executes it when the
device is powered up (whether via auxillary power or main power).

The RX RISC CPUs use a basic subset of MIPS, which seems vaguely MIPS I-ish;
moreover, they do not have a hardware multiplier.

Despite the name, the “RX CPU” does not handle packet reception in any shape or
form. The naming appears to be a legacy artifact; it appears that in prior
generations of Broadcom network controllers, there was an “RX CPU” handling
reception and a “TX CPU” handling transmission; however, over successive
generations, TX and RX were moved into hardware, and the TX CPU was removed.
The functions implemented by the RX CPU bootcode on the BCM5719 are highly
miscellaneous; it is likely that they are, essentially, the “dregs” of assorted
random functionality which, quite simply, nobody wanted to bother translating
into RTL, and for which there was no particular pressing need to either. One
core is more than enough to implement this remaining functionality, so it
figures they chose one core to scrap (the TX CPU), one core to keep (the RX
CPU), and never bothered renaming it. (The RX CPU's registers came first in the
register map in prior generations, implying it is the “first” CPU, which
explains why they chose the RX CPU to keep and not the TX CPU.)

The Bootcode is split into *stage 1* and *stage 2*. The boot ROM, a mask ROM
burnt into the chip, loads the stage 1 (hereafter S1) bootcode from NVM and
executes it. The S1 performs a lot of device initialization, and then
chainloads the stage 2 (S2) from NVM and executes it. The S2 performs some
execution and then executes a main loop perpetually.

The S1 code, for example, is responsible for loading the MAC addresses from NVM
and setting them into the correct device registers. The S2 code is less
critical and more random; it implements parts of Wake-on-LAN support, and
implements the PCI Vital Product Data (VPD) capability (which allows the host
to query PCI VPD information, which contains things like serial numbers and
asset tags).

The Bootcode is necessary to device function. Strictly speaking, this is
probably not necessary; the Linux driver for the BCM5719 (amongst other
devices), `tg3`, for example, won't work without the bootcode because it
expects to see a magic value in device memory that indicates that the bootcode
has successfully booted, and will not bring up the device without it. Moreover
it is likely that the `tg3` driver does not possess the necessary logic to load
MAC addresses from NVM itself, expecting them to be loaded by the device
automatically. The important point is that the bootcode does not have access to
any state not accessible by the host; all of the registers mutated by the
bootcode are equally accessible to the host, as are the contents of NVM, etc.

Because the BCM5719 has a limited version of the Bootcode burnt into ROM (the
“self-boot” option), it would be theoretically possible to activate use of this
built-in Bootcode. This may be of interest to persons who object to proprietary
firmware stored in mutable storage media, but do not object to firmware stored
in truly immutable ROMs. No research has been performed on how to enable the
self-boot image, though such research is unlikely to prove difficult were there
demand for it.

**APE Firmware.** The BCM5719 contains four MIPS RX CPUs, one for each PCI
function (port); it also contains a single “APE” core, which is a little-endian
ARM CPU. This core exists to run value-add applications such as firmware to
enhance system manageability. It appears that the functions implemented on the
APE have changed over the years; it is clear that the APE was once used to
implement ASF support, but the BCM5719 does not support ASF. On the BCM5719,
the APE firmware is used to implement NCSI support. (It is unclear whether APE
firmware implementing any other functionality is available for the BCM5719; the
NCSI APE image is the only one which has been observed).

(NCSI is a method by which the use of a network controller can be shared,
generally by a BMC. A BMC is connected to a NCSI-supporting network controller
chip using either a variant of the RGMII interface or SMBus, and the network
controller arbitrates access to the network interface between the host and the
BMC. This allows the BMC, which implements manageability functions for the
system, to piggyback on the same Ethernet ports. This avoids the need for a
separate Ethernet port (and switchport) for the BMC.)

The APE firmware for the BCM5719 is specifically labelled as providing the
“NCSI” functionality of the device, and is presumably essential to NCSI
functionality. The BCM5719 APE firmware is believed to be nonessential to the
mere use of the device as a network controller by the host system, though this
has yet to be tested.

### Project Objective

The objective of the *RTG* (Reversed Tigon) subproject is to generate an open
specification describing the technical requirements of the Bootcode and APE
firmware, so as to enable free software replacements to be written. This
documentation is produced so as to facilitate a cleanroom reimplementation
process.

For the time being, the RTG subproject focuses on the Bootcode, because it is a
far more tractable challenge. The APE firmware is substantially larger, and far
less is known about it; whereas the memory and register maps of the BCM5719 are
mostly-completely reproduced in the register manual, nothing whatsoever is
known of the APE's memory maps or registers. In fact, it was only recently
discovered that the APE uses the ARM architecture.

### Fucking Broadcom

Jesus christ, Broadcom. What is with this company? They publish a register
manual freely, which on the one hand is incomplete (there are things the Linux
tg3 driver does with the device which aren't described anywhere in it; in fact,
the register manual itself literally tells you to go and look at the tg3 driver
as a guide), and on the other hand describes a lot of assorted device innards
nobody who's simply writing a driver should need to go near.

They don't publish any documentation as to the layout of data and code in NVM,
for heavens knows why, yet at the same time, they don't seem to consider the
layout a secret, because it (or at least parts of it) are coincidentially
defined as a struct inside a C header file for a Broadcom shared library for
interfacing with various aspects of their NICs (ironically, the NVM does not
appear to be one of those aspects).

They don't publish their datasheets; they're *secret*. They publish a register
manual which describes the most ungodly innards of the device, but they don't
want you to know the pinout.

But these are tinkerer's concerns. What about users? Well... bizarrely, the
experience sucks even for users. Let's take an example: you install a BCM5719
card in a Windows machine. You need to install a driver. You just go to their
website and download it, right? This proved surprisingly difficult, because the
Windows driver they provide on their website is literally just INF/SYS/CAT
files without an installer — despite the fact that I know for a fact that such
an installer exists, because you can find it being distributed on OEM websites,
it presumably having been given to them by Broadcom. So to even get the device
working on Windows, you have to know how to install a driver without an
installer, which isn't at all obvious.

Then we have Broadcom's diagnostic utilities. I literally gave up on getting
these working. These utilities interface directly with the device, so there's a
DOS version and also a UEFI version. Except that both the “DOS” and “UEFI”
versions of their diagnostic tools provided on their website are... the DOS
version; it seems almost like someone accidentially uploaded the wrong file
when uploading the “UEFI” version. Even more hilariously, I know for a fact
that a Windows version of these diagnostic tools exists, yet you won't find
Broadcom distributing it anywhere.

Broadcom seems to be very unfamiliar with the concept that they need to
actually provide — for download — on their website — software and documentation
to enable people to actually use their products. When they do, it seems like an
unfamiliar experience to them, like they don't really know how to do it right,
and what they do provide is a completely random subset of what they provide to
OEMs, bearing no correlation to any comprehensible set of criteria as to what
to publish and what to not. Moreover, what is the point of not publishing
materials which one not only provides to OEMs, but authorizes to distribute to
end users? You can find BCM5719-related software freely available for download
on the websites of OEMs who sell expansion cards or servers which use BCM5719s,
but which isn't on Broadcom's website. What does this accomplish?

If you need Broadcom drivers/software/that Windows version of their diagnostic
tools/etc., your best bet is probably to go nuts with a search engine until you
find what you're looking for on the FTP server of some major server OEM. If
only I were kidding.

### Fucking Broadcom, Part II

No, screw it, I'm going to rant to completion, so let's rant about a different
business unit: Broadcom's switch chips. The whitebox, merchant silicon
switching movement is interesting, and Broadcom's chips (e.g. the now eponymous
Trident II) are very popular. Of course Broadcom doesn't publish any
documentation for these chips whatsoever — that would make it too easy to use
their products. Although I'm not sure they actually give register manuals for
those chips to anyone — or possibly don't even *have* register manuals, it
seems like they make people use some proprietary SDK to interface with the
chips instead. AFAICT it used to be they weren't handing this proprietary SDK
out to just anyone. However, it appears it has finally occurred to them that
making it hard for people to use their products is not a winning strategy, and
so they now provide a freely available SDK for programming their switch chips.
This library is hilariously called “OpenNSL”, even though it is a) a
closed-source 56 MiB binary shared library and b) Broadcom-specific; to Broadcom,
the idea of publishing the SDK so that people can actually use their products
is truly *radical*.

I find it interesting to note that Mellanox is apparently now getting the
drivers for their merchant silicon switch chips, which compete with Broadcom's,
mainlined in the Linux tree (mlxsw); it's nice that at least one company in
this market seems to have some sensibility.

### Fucking Broadcom, Part III

Oh yes, and I'm fairly sure open source Wi-Fi driver developers have had tons
of trouble with Broadcom over the years. Not to mention the fact that the
Raspberry Pi is still stuck with some Broadcom-provided blob for a bootloader.

Okay, I'm done. I now return you to your regularly scheduled technical
specification.

## High-Level Overview

### Incorporated Resources

This specification **must** be read in association with the following resources:

  - The [register manual published by Broadcom]();

  - The **resource definition file** provided with this specification. This
    provides machine readable definitions for registers and other numbered
    resources; this specification text does not duplicate information available
    in the resource definition file.

    The resource definition file does not document all registers; for many
    registers you must still refer to the Broadcom manual. However, in
    particular, it aims to specify all registers which are actually accessed by
    bootcode, as well as all registers which are not documented in the Broadcom
    manual but whose existence or meaning has been inferred or guessed. It also
    documents device resources other than registers, such as the NVM layout.

    The resource definition file is maintained in Git as [regs.yaml](), but for
    reference purposes you will probably prefer to use the pretty [XHTML
    Resource Listing file](regs.xhtml) generated from it.


### The NVM Image

The purpose of this specification is to specify how the contents of NVM must be
formulated — and, by extension, how every configuration block and code image
which must be present in NVM must be formulated. The specification thus starts
by discussing the layout of NVM, and then we zoom into specific subcomponents
of that NVM layout, such as the bootcode.

All fields in NVM are big endian unless otherwise specified.

The NVM layout begins with the NVM header at the very start of the NVM device
(offset 0). The NVM header is as specified in the [Resource
Listing](regs.xhtml#NVM) and should follow the format specified there. Note
that all device functions (ports) consume the same NVM device, NVM header and
bootcode.

### Stage 1 Bootcode

The fields of the NVM header which are relevant to the RX CPU boot ROM and how
it boots the stage 1 bootcode are the “Boot” fields; the five words at the very
beginning of the NVM device. These are the fields used by the boot ROM to
locate and load the S1 image.

The recommended load address for the S1 image is `0x0800_3800`.

**CPU Architecture.** The S1 and S2 bootcode images are big-endian 32-bit MIPS
ISA images. For a guideline of which revision of MIPS to target, MIPS I appears
to be a reasonable ballpark — the precise capabilities of the RX CPU are not
known. The RX CPU lacks hardware multiply or divide functionality, so the
machine code must not use multiply or divide instructions.

**Entrypoint and Version String Pointer.** Immediately following the NVM header
(at the byte offset specified by the NVM Boot Stage 1 Offset field, which in
all known circumstances should be 0x28C) comes the first instruction of the S1
image's entrypoint. The RX CPU Boot ROM loads the image such that this
instruction is located at the Boot Stage 1 Load Address precisely; the NVM
header is not loaded into device memory.

This first instruction must be a jump instruction, followed by a NOP. This is
required because the third word of the S1 image must be a pointer to a
zero-terminated ASCII string embedded in the S1 image describing the image
version. This pointer is encoded like any pointer; it assumes that the S1 image
was loaded at the address specified by the Boot Stage 1 Load Address field of
the NVM header. Thus, subtract the Boot Stage 1 Load Address value from the
value of the pointer word, and add the resulting value to the value of the NVM
Boot Stage 1 Offset field to get the offset of the version string.

**Entrypoint tasks.** After the initial jump over the version pointer, the
immediate concern of the entrypoint code is to initialize whatever runtime
support is necessary to the execution of the S1 bootcode proper. Assuming C is
used as the implementation language, this should simply mean initializing the
stack by setting the correct values for the `$sp` and `$fp` registers. These
registers should both be set to the end of the region of memory reserved for
the stack. The recommended end of stack address is `0x0800_7000`. The
entrypoint may then jump to the entrypoint function of the S1 bootcode proper,
which does not return under any circumstance.

**Image size and CRC.** The amount of data copied to the region of memory
starting at the Boot Stage 1 Load Address by the RX CPU Boot ROM is determined
by the NVM Boot Stage 1 Size field, which specifies how many 32-bit words (not
bytes) comprise the S1 image. The word immediately following the last word of
the S1 image which is loaded into memory by the RX CPU Boot ROM (that is, the
word at `('Boot Stage 1 Offset' + 'Boot Stage 1 Size' * 4)`) contains a CRC
over the words of the S1 image. The CRC is a standard Ethernet CRC, and covers
all of the words of the image which are loaded into memory by the RX CPU Boot
ROM; that is, it starts with the first byte of the entrypoint instruction, and
ends with the byte immediately preceding the CRC word. This CRC is verified
by the RX CPU Boot ROM and must be correct.

**Boot header CRC.** The “Boot” fields of the NVM Header are also protected by
an Ethernet CRC, which must als be correct in order for the RX CPU Boot ROM to
load the S1 image. The CRC value is stored in the NVM Boot Header CRC field,
and is calculated over the Boot Magic through Boot Stage 1 Offset fields
inclusive.

**Function.** The S1 image performs initialization tasks which will be discussed
subsequently. After performing its initialization tasks, the S1 image then
chainloads the S2 image.

### Stage 2 Bootcode

**Image Format.** Similarly to the S1 image, the S2 bootcode image is comprised
of a header, followed by the code and data, followed by a CRC word. The S2
Header is simpler and has the following form:

    4  ui  Magic (HEADER_MAGIC; 0x669955AA)
    4  ui  S2 Image Size in Bytes

**Load Address.** Like the S1 image, the S2 image is not relocatable. The image
header does not specify the load address; the S1 image must know (have
hardcoded) the correct load address; in any case, the S2 image should always be
loaded at `0x0800_0000`. Thus, the S2 image is loaded before the S1 image in
memory. The base address of the S1 image in memory must be chosen to provide
adequate room for the S2 image.

**Entrypoint.** The entrypoint code of the S2 image follows immediately. Like
the S1 image, this comprises a jump instruction followed by a NOP instruction,
which jumps over a version string pointer word. However, the version string
pointer word is vestigial and is set to zero. The `$sp` and `$fp` registers are
reinitialized to the end of the stack; the S2 image uses the same region of
memory for the stack as the S1 image. The entrypoint code then calls into the
S2 Bootcode proper, which does not eturn under any circumstance.

**CRC.** The S2 CRC word appears immediately after the S2 code. It is a
standard Ethernet CRC calculated over the S2 code, **not** including the header
or CRC word. In other words, the first byte covered by the CRC is the first
byte of the first instruction word of the entrypoint code, and the last byte
covered is the byte immediately preceding the S2 CRC word.

**Image Size.** The S2 Image Size field in the S2 Header is in bytes. It does
NOT include the size of the S2 Header itself **but does include size of the the
CRC word.**

**Image Location in NVM.** The S2 image is always located *immediately* after
the S1 image in NVM; that is, the S2 Header Magic is the word immediately
following the last word of the S1 Image, which is the S1 Image CRC word. The S2
image is located for loading by the S1 bootcode by adding the value of the NVM
Boot Stage 1 Size field to the value of the NVM Boot Stage 1 Offset field.

**Function.** The S2 image is the second and final image loaded into memory for
execution by the RX CPU. After being chainloaded by the S1 image, it performs a
small amount of additional initialization and then falls into a main loop.
The RX CPU then executes this main loop for all eternity, or until the device
is reset or loses both main and auxillary power.

### Additional Images

The Directory Entries in the NVM Header enable additional images to be
specified. Many of these relate to Option ROMs or code modules or configuration
data loaded by Option ROMs.

**APE Code.** The only Directory Entry type believed to be of
innate relevance to the device itself is the APE Code type, which appears to be
used by the APE Boot ROM (which is as yet still unknown) to load the APE Code
at bootup. The boot of the APE is autonomous and is not in any way orchestrated
by any of the RX CPUs nor by the host.

**PXE Option ROM.** The device does not use this image itself. However, the S1
image does search for this directory entry so that it can configure
undocumented device registers with the location of the PXE Option ROM, if it is
found, thereby enabling the host to access it via the standard PCI mechanism.

**Extended VPD.** If this directory entry is found, the bootcode uses the area
of NVM specified by this directory entry to service requests to the PCI VPD
capability. If it is not found, it uses the VPD block embedded in the NVM
header.

### APE Code

The APE Code image (as pointed to by the APE Code directory entry) consists of
a header and several compressed segments. The header is believed to be of fixed
length and specifies the offsets, sizes and load addresses of these segments.

The first segment contains unknown data which appears to be a set of key-value
properties, with ASCII keys and binary values. The format is otherwise unknown.

The second segment is the ARM code to be executed by the APE. The Thumb 2
encoding is used.

The third and fourth segments appear to be data segments used by the program.

Each segment is separately compressed using the LZS Compression Algorithm
described herein.

### LZS Compression Algorithm

Some data (such as the segments of an APE Code image, or parts of non-UEFI PXE
Option ROMs) is compressed using a custom compression format, which is a simple
implementation of
[LZS](https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Stac). (UEFI
images are compressed using the UEFI standard compression algorithm.)

A stream of LZS-compressed data logically consists of a series of chunks. Each
chunk is either a literal byte or a reference. A literal byte causes a
specified byte to be output; it is also appended into the dictionary buffer. A
reference chunk is a reference to a slice of the dictionary buffer, expressed
as an offset in bytes from the start of the buffer, and the number of bytes to
take from the buffer; these bytes are then output, and are also appended again
into the dictionary buffer just as literal bytes are. Note that references are
always absolute offsets from the start of the dictionary buffer; they are in no
way relative to the current position or the amount of data that has been
appended into the dictionary buffer. The dictionary cursor points to a byte in
the dictionary buffer; when a byte is added to the dictionary buffer, the byte
is written to the location pointed to by the cursor, and the cursor is
incremented. If the cursor reaches the end of the dictionary buffer, it is
reset to the beginning of it; thus, the dictionary buffer is used as a
primitive ring buffer. The dictionary buffer is 2048 bytes in size. Initially,
the dictionary cursor points to offset 2014 (where offset 0 is the first byte
of the dictionary buffer). Bytes 0 through 2013 inclusive of the dictionary
buffer are initialized to 0x20, and the remaining bytes are initialized to
0x00.

The encoding format is as follows: First, a control byte is read. The eight
bits of this control byte indicate the types of the eight chunks following
the control byte. A 1 indicates a literal byte chunk, and a 0 indicates a
reference chunk. To determine the type of the next chunk, the LSB of the
current control byte value is shifted off and read. This is repeated until
eight chunks have been consumed since last reading a control byte; a new
control byte is then read, which specifies the types of another eight
chunks, the LSB again specifying the type of the chunk coming first.

When the type bit shifted off the control byte indicates a literal byte chunk,
read a byte from the stream, output it as decompressed data, and append it to
the dictionary buffer.

When the type bit indicates a reference chunk, read two bytes. The first of
these bytes shall be called B0 and the second shall be called B1. The reference
offset (in bytes from the beginning of the dictionary buffer) is formed from B0
(which provides the low 8 bits of the offset), and the 3 high bits of B1, which
provides bits 8-10 of the offset. The number of bytes to take from the
dictionary buffer starting at that offset — the reference length — is
determined by taking the low 5 bits of B1 and adding 2 to the resulting value.
Each byte in the contiguous sequence of bytes in the dictionary buffer
referenced by this offset and length is output as decompressed data, and also
appended again into the dictionary buffer as would be a literal byte.

A concise summary of the format follows:

    8  bits  Control Byte
    Each iteration:
      Shift the LSB off the control byte
        (if you have already shifted out all eight bits, read a new control byte)
      If it is 1, read a literal:
        Read a byte, output it, append it to the dictionary buffer
      Else, read a reference:
        Read a byte (B0)
        Read a byte (B1)
        OFFSET = B0 | ((B1 & 0xE0) << 3);  # Offset from start of dictionary buffer
                                           # in bytes.
        LENGTH = (B1 & 0x1F) + 2;          # Number of bytes to take from dictionary
                                           # buffer, starting at OFFSET.

        For each byte in the range in the dictionary buffer specified by OFFSET and
        LENGTH, output that byte and append it to the dictionary buffer.

    2048 byte dictionary buffer, cursor initially set to offset 2014. Cursor
    incremented modulo 2048 after a byte is written.
    Initial state: Bytes 0-2013 inclusive set to 0x20, bytes 2014-2047 set to 0.

In ABNF (but with bits, not bytes):

    BYTE        = 8BIT
    TypeBit     = BIT       # 1 indicates literal

    Literal     = BYTE
    RefOffset   = 11BIT
    RefLength   = 5BIT      # Add 2 to value
    Reference   = RefOffset RefLength
    Chunk       = Literal / Reference

    Stream = *(8TypeBit 8Chunk) 8TypeBit 0*8Chunk

## Miscellaneous Constants

HEADER_MAGIC:          0x669955AA.

DRIVER_READY_MAGIC:    0x4B657654 ("KevT").

BOOTCODE_READY_MAGIC:  0xB49A89AB (~"KevT").

BIOS_DISABLED_MAGIC:   0xDEADDEAD.

## Common Procedures Functional Description

### Procedure: Sleep

Given an argument `delay`, note the initial value of `REG_TIMER` and loop for
as long as the current value of `REG_TIMER` is less than that value plus
`delay`.

### Procedure: MII Wait

This waits for a previously initiated MII MDIO register access operation to
complete.

Loop for as long as `REG_MII_COMMUNICATION__START_BUSY` is set.

### Procedure: MII Read

This procedure takes the following arguments: PHY Number, a 5-bit PHY port
number; Register Number, a 5-bit MII register number.

  0. Call the MII Wait procedure.

  1. Set `REG_MII_COMMUNICATION`, such that the PHY Number and Register Number
     are set into the correct fields; the Start/Busy bit is set; and the
     Command field is set to Read.

  2. Call the MII Wait procedure.

  3. The procedure returns the low 16 bits of `REG_MII_COMMUNICATION` as the
     result.

### Procedure: MII Write

This procedure takes the following arguments: PHY Number, a 5-bit PHY port
number; Register Number, a 5-bit MII register number; Value, a 16-bit value to
write to the register.

  0. Call the MII Wait procedure.

  1. Set `REG_MII_COMMUNICATION`, such that the PHY Number, Register Number
     and Value are set into the correct fields; the Start/Busy bit is set;
     and the Command field is set to Write.

Note: The procedure does not wait for the write to complete. However,
there is no need to call it explicitly after calling the MII Write procedure,
because any further invocation of a MII Read or MII Write procedure will
perform such a wait.

### Procedure: MII Select Block

This is used to select a block of registers for PHYs that implement the 0x1F
block select register. This is used by SERDES PHYs and appears to also be used
by the mysterious MIIPORT 0.

This procedure takes the following arguments: PHY Number, a 5-bit PHY port number; Block Number, a 16-bit number.

  0. Call the MII Write procedure with the given PHY Number, register number
     0x1F, and the Block Number as the value.

### Procedure: NVM Acquire

  0. Set `REG_SOFTWARE_ARBITRATION` to `REQ_SOFTWARE_ARBITRATION__REQ_SET0`.

  1. Loop until `REG_SOFTWARE_ARBITRATION__ARB_WON0` is set.

  2. If unknown, reserved bit 27 of `REG_NVM_CONFIG_1` is not set, set
     `REG_NVM_ACCESS__ENABLE` and clear all other bits in that register.

### Procedure: NVM Release

  0. If unknown, reserved bit 27 of `REG_NVM_CONFIG_1` is set,
     set `REG_NVM_ACCESS` to 0.

  1. Set `REG_NVM_SOFTWARE__ARBITRATION` to
     `REG_SOFTWARE_ARBITRATION__REQ_CLR0`.

### Procedure: NVM Acquire and Release

  0. Call the NVM Acquire procedure.

  1. Set `REG_NVM_COMMAND` to 0.

  2. If unknown, reserved bit 27 of `REG_NVM_CONFIG_1` is not set,
     set `REG_NVM_ACCESS` to zero.

  3. Set `REG_SOFTWARE_ARBITRATION__REQ_CLR0`.

  4. Finally, the result of the procedure is the current value of
     `REG_NVM_ACCESS`.

### Procedure: NVM Wait

  0. Loop until `REG_NVM_COMMAND__DONE` is set.

### Procedure: NVM Read 32

This procedure takes the following arguments: a 32-bit unsigned byte offset
value, an Acquire Flag, and a Release Flag.

  0. If the Acquire Flag is set, call the NVM Acquire procedure.

  1. Call the NVM Translate Offset procedure with the given byte offset value;
     the result is then truncated to the low 24 bits; this value is called the
     Translated Byte Offset.

  2. Set `REG_NVM_ADDRESS` to the Translated Byte Offset.

  3. Set `REG_NVM_COMMAND` such that `REG_NVM_COMMAND__FIRST` is set, `REG_NVM_COMMAND__LAST` is set,
     `REG_NVM_COMMAND__DONE` is set (this is a W2C bit; setting it clears the done bit from any
     previous NVM operation) and `REG_NVM_COMMAND__DOIT` is set; set all other bits to 0.

  4. Loop until `REG_NVM_COMMAND__DONE` is set.

  5. The 32-bit word which was read is in `REG_NVM_READ`. This shall be the
     result of the procedure.

  6. If the Release Flag is set, call the NVM Release procedure.

### Procedure: NVM Read Bulk

### Procedure: NVM Write 32

### Procedure: NVM Write Bulk

### Procedure: NVM Find Directory Entry

This procedure takes an argument: the desired directory entry type, an 8-bit
value.

Using the NVM Read 32 procedure, read the Type/Size word of each entry in the
directory in NVM, starting with the first entry. The array index of the first
matching entry is returned; if no entry matches, -1 is returned.

A directory entry is considered a match if

  a. the low 22 bits of the Type/Size word are nonzero, and

  b. the high 8 bits of the Type/Size word match the desired directory entry
     type argument.

### Procedure: NVM Translate Offset

This procedure takes an argument: a 32-bit byte offset value.

  0. Determine whether 264-byte pages are in use by calling the NVM Determine
     If 264-Byte Pages Are In Use procedure. If the procedure returns that
     264-Byte Pages are not in use, return from this procedure now, returning
     the passed byte offset value unchanged.

  1. Calculate the byte offset value divided by 264 (rounding down to the
     nearest integer), then multiplied by 512, then add the remainder of the
     division. This is the result.

### Procedure: Get PHY Port Number

  0. Call procedure Determine PHY Type.

     If the procedure indicates that the GPHY PHY is in use, the PHY port
     number is the Function Number (call procedure Get Function Number) plus 1.

     If the procedure indicates that the SERDES PHY is in use, the PHY port
     number is the Function Number (call procedure Get Function Number) plus 8.

### Procedure: Get Function Number

  0. Return `REG_STATUS__FUNC_NUMBER`.

### Procedure: Determine PHY Type

  0. If `REG_SGMII_STATUS__MEDIA_SELECTION_MODE` is set, the SERDES PHY is in
     use. Otherwise, the GPHY PHY is in use.

### Procedure: Determine APE Port Number

  0. Call procedure Get Function Number to determine the function number, and
     use the resulting value as an index into the following constant array of
     8-bit values:
 
       APE_PORT_PHY0,
       APE_PORT_PHY1,
       APE_PORT_PHY2,
       APE_PORT_PHY3.

    The value at the given index in the above array is the APE Port Number.

## S1 Bootcode Functional Description

The entrypoint assembly must be constructed as described in previous sections,
and must call the S1Start procedure described in this section.

Rather than describing the exact sequence of steps taken by the original
firmware, this is a list of things which the S1 firmware seems to need to do.
This list is a work in progress.

The following actions are taken early, immediately after S1 boot.

  - Or `REG_MEMORY_ARBITER_MODE__ENABLE`.
  - Mask `REG_RX_RISC_MODE__ENABLE_DISABLE_CACHE`.
  - Or `REG_PCI_STATE__{APE_PROGRAM_SPACE_WRITE_ENABLE,APE_SHARED_MEMORY_WRITE_ENABLE,APE_CONTROL_REGISTER_WRITE_ENABLE}'.

The following actions are taken during S1 init.

  - Mask `REG_GPHY_CONTROL_STATUS__{BIAS_IDDQ,GPHY_IDDQ,SGMII_PCS_POWER_DOWN}`. If `REG_STATUS__VMAIN_POWER_STATUS` is asserted, mask `TLP_CLOCK_SOURCE`, otherwise or it.
  - Load various config from NVM and store it in memory for later use.
  - Load various config from NVM and stash it into registers. For example:
      - MAC0 is stored into `REG_PCI_SERIAL_NUMBER_{LOW,HIGH}`.
      - MAC0,1,2,3 are set into `REG_EMAC_MAC_ADDRESSES_{0,1,2,3}_{HIGH_LOW}`. Also set `GEN_MAC_ADDR_{HIGH,LOW}_MBOX`.
      - `REG_PCI_STATE__EXPANSION_ROM_DESIRED` is set/masked based on whether expansion ROM is found in NVM directory. If found
      - `REG_PCI_POWER_{DISSIPATED,CONSUMPTION}_INFO` are set from the corresponding fields in NVM.
      - `REG_PCI_POWER_BUDGET_{0,...,n}` are initialised from NVM.
      - Set `REG_PCI_SUBSYSTEM_ID` to the subsystem and subsystem vendor ID from NVM. There are different fields in NVM for each PCI function and based on whether GPHY or SERDES is in use.
      - Set `REG_PCI_VENDOR_DEVICE_ID` to vendor/device ID from NVM.
      - Set `REG_PCI_CLASS_CODE_REVISION` as desired, partly from `REG_CHIP_ID`.
  - Lots of gencom regions are zeroed during init, though not all.
  - Other miscellaneous register init:
      - Mask REG 0x64C0 bits 0x7FF, or bits 0x0010. This register is unknown.
      - Set unknown REG 0x64C8 to 0x1004.
      - Set `REG_CLOCK_SPEED_OVERRIDE_POLICY` to `MAC_CLOCK_SPEED_OVERRIDE_ENABLE`.
      - Mask REG 0x64DC bits 0x0F, or bits 0x01. Unknown.
      - Mask REG 0x64DC bits 0xC00, set ...
      - Unknown stuff involving REG 0x6530, REG 0x65F4, depends on config
      - `REG_LSO_NONLSO_BD_READ_DMA_CORRUPTION_ENABLE_CONTROL`: Set BD and NonLSO fields to 4K.
      - Mask `REG_GPHY_STRAP__{RXMBUF,TXMBUF,RXCPU_SPAD}_ECC_ENABLE`.
      - Initialise `REG_LED_CONTROL` as desired.
      - Set `REG_MISCELLANEOUS_LOCAL_CONTROL` as desired. Default setting seems to be to set GPIO0 as output, on. Other GPIOs unchanged.
        the offset of it in NVM is set into `REG_EXPANSION_ROM_ADDRESS`.
      - Set `REG_EAV_REF_CLOCK_CONTROL` as desired. This is initialised from
        `CFG_HW`; the `TIMESYNC_GPIO_MAPPING`, `APE_GPIO_{0,1,2,3}` fields
        within it are copied to the corresponding fields in
        `REG_EAV_REF_CLOCK_CONTROL`.
      - Optionally enable `REG_GRC_MODE_CONTROL__TIME_SYNC_MODE_ENABLE`.
      - Or `REG_MII_MODE__CONSTANT_MDIO_MDC_CLOCK_SPEED`.
      - Set or clear `REG_GPHY_CONTROL_STATUS__SWITCHING_REGULATOR_POWER_DOWN` as desired.
      - Set or clear `REG_TOP_LEVEL_MISCELLANEOUS_CONTROL_1__NCSI_CLOCK_OUTPUT_DISABLE` as desired.
    - Lots of MII init. MII init done on FUNCTION 0 ONLY:
      - MIIPORT 0 (0x8010):0x1A |= 0x4000
      - MIIPORT 0 (0x8610):0x15, set bits 0:1 to 2. (Note: This is done in a
        retry loop which verifies the block select by reading 0x1F and
        confirming it reads 0x8610, and then verifies that bits 0:1 have been
        set to 2, and retries about a dozen times until the block select and
        write are both correct. Probably an attempt to work around some bug or
        weird asynchronous behaviour for these unknown MII registers.)
      - MIIPORT 0 (0x8010):0x1A, mask 0x4000.
    - MII init for all functions (MIIPORT determined by function/PHY type):
      - Set `MII_REG_CONTROL` to `AUTO_NEGOTIATION_ENABLE`.

The following actions are taken late, immediately before S2 boot.

  - Set `REG_BUFFER_MANAGER_MODE` to `ENABLE|ATTENTION_ENABLE|RESET_RXMBUF_PTR`.
  - Set GENCOM 0x004 to `DRIVER_READY_MAGIC`.

Note: The above is an incomplete description of what stands out as the most
important-looking stuff taken during S1 init. Keep in mind that all of these
registers are visible to host drivers too; the MIPS bootcode basically feels
like an autoconfiguration system. In principle if you don't want stuff like WoL
the host driver could work autonomously. Not sure if the host drivers know how
to read the MAC from NVM and set it automatically, etc.; probably not. But in
principle unlike e.g. the APE side it's probably not going to be that hard to
mash enough of this together to get packets flowing. The host drivers will
refuse to load if they don't detect the firmware's up via some gencom
mailboxes, but they don't seem to demand that much beyond that. The above
doesn't mention much about gencom, and a lot of gencom seems to be used for
applications for which private globals would be better (in that gencom exists
for firmware-host communication but a lot of the fields of it seem to go
unused.) But there might also be software which uses gencom fields which I
don't know about (e.g. diag tools), so assuming any gencom fields are unused
might not be safe. But really, for the time being, I'd just make globals
private by default and put stuff in GENCOM if you can find evidence of
dependence in e.g. the Linux tg3 driver. The `mailboxes` are the most common
case, the driver demands the magic be there or it won't init (see
`GEN_FIRMWARE_MBOX` below). See tg3.

## S2 Bootcode Functional Description

The entrypoint assembly must be constructed as described in previous sections,
and must call the S2Start function described in this section.

Incomplete list of MII initialisation done by S2 init code FOR GPHY:
  - Set `MII_REG_CONTROL` to `RESET`; wait until `RESET` bit clears.
  - Mask `REG_MDI_CONTROL` bits 0x4. Meaning unknown.
  - Set MII (SHADOW 0x05):0x1C as desired. e.g. `CLK125_OUTPUT_ENABLE`, `SIGDET_DEASSERT_TIMER_LENGTHEN`, `DISABLE_LOW_POWER_10BASET_LINK_MODE`, `LOW_POWER_ENC_DISABLE`.
  - Cases for handling Mini-PCI mode (YES REALLY). Not mentioned here.
  - Set `MII_REG_1000BASE_T_CONTROL` as desired (e.g. `ADVERTISE_FULL_DUPLEX|ADVERTISE_HALF_DUPLEX`).
  - Set `MII_REG_AUTO_NEGOTIATION_ADVERTISEMENT` as desired (pretty much enable
    `PROTOCOL_SELECT__IEEE_802_3` and all the full/half duplex options).
Incomplete list of MII initialisation done by S2 init code FOR SERDES:
  - Set `MII_REG_CONTROL` to `AUTO_NEGOTIATION_RESTART|AUTO_NEGOTIATION_ENABLE`.

Incomplete list of initialisation taken by S2 init code:
  - Set `REG_EMAC_MODE__PORT_MODE` based on `REG_STATUS__ETHERNET_LINK_STATUS`
    (GPHY) or `REG_SGMII_STATUS__LINK_STATUS` (SERDES).
  - Set `REG_LINK_AWARE_POWER_MODE_CLOCK_POLICY__MAC_CLOCK_SWITCH__6_25MHZ`.
  - Set/mask `REG_CPMU_CONTROL__LINK_{AWARE,IDLE,SPEED}_POWER_MODE_ENABLE` as
    desired (from NVM).

The following actions are taken late in init, immediately before the S2 main loop:
  - Mask `REG_CLOCK_SPEED_OVERRIDE_POLICY__MAC_CLOCK_SPEED_OVERRIDE_ENABLE`.
  - Set `GEN_FIRMWARE_MBOX` to `BOOTCODE_READY_MAGIC`.

Then the main loop happens.

## The APE

The APE is a Cortex-M3 running in little-endian mode. There is one APE per
entire chip (not per port) and it serves all ports. It has direct access to a
number of peripherals reserved exclusively for it and which cannot be used by
the MIPS CPUs. It also shares a few peripherals with the MIPS CPUs and host
(e.g. NVM access).

Most of the APE's memory and register space is wholly separate from the ranges
exposed to the MIPS cores and the host and cannot be accessed except by
obtaining control over the APE core.

The APE is used to implement NC-SI, and processes and copies frames in software
from/to the network to/from a BMC attached by NC-SI RGMII or SMBus. Because
this is done in software by a Cortex-M3, it has pretty poor performance, so be
it.

Examples of peripherals available to the APE include:

  - Some sort of debug UART on the chip (haven't looked at this)
  - SMBus (one of the options for NC-SI, haven't looked at this)
  - NC-SI RGMII (one of the options for NC-SI, now well understood)
  - Lock/Arbitration Registers (shared with MIPS cores, host, etc.)
  - NVM Access Registers (shared with MIPS cores, host, etc.)
  - Network TX/RX ranges (used to TX/RX to/from the network ports, one range for each port)
  - Host-to-BMC/BMC-to-host ranges (not yet understood but appears to work similarly)
  - APE RX Management Filters (controls what RX traffic gets forwarded to the APE)
  - A few GPIOs (the "APE GPIOs")

There is a small amount of shared memory for each function, which the APE uses
to communicate with each function. The shared memory region for port x is
available to the APE and the MIPS core for that port. These shared regions are
also all available to the host.

The SHM region for function 0 is used predominantly for communication between
the APE and the host, and also by the APE to store various bookkeeping
information that should be preserved between resets of the APE. SHM regions for
functions 1, 2, 3 use the same layout, but are barely used with only the region
for APE-to-MIPS/MIPS-to-APE communication being used.

For implementing NC-SI, the following areas of functionality must be implemented:

  - TX To Network
  - RX From Network
  - TX To RGMII
  - RX From RGMII
  - Initialization (especially of management filters, else RX from net won't work)
  - NC-SI Control Packet Handling (see the DMTF NCSI specification)

Let's discuss each of these units.

### TX To Network

The following memory ranges are involved in TX to network:

  - `0xA002_0000 + 0x2000*port`, size `0x2000`, `0 <= port < 4` (i.e.,
    `[0xA002_2000, 0xA002_3FFF]` for the second port, etc.).

    This is the memory range into which frames to transmit to the network
    must be written. It is *write-only*; reads will return all-zeroes,
    don't bother trying it.

  - `REG_APE__TX_TO_NET_POOL_MODE_STATUS`.
  - `REG_APE__TX_TO_NET_BUF_ALLOC`.
  - `REG_APE__TX_TO_NET_DOORBELL`.

The DOORBELL register is written after you've finished squirting a frame into
the write-only region, with an encoded location of the frame in that region.

The BUF_ALLOC and POOL_MODE_STATUS registers control allocation of that
write-only region. The region is allocated in blocks of 128 bytes
(`NET_BLOCK_SIZE`), and frames that take more than one block must be spanned
over multiple blocks. The allocation of blocks is done by hardware, not
software, using these registers.

**Initialization.** Before you can TX to network, you need to initialize the resources:

  - Set `REG_APE__TX_TO_NET_POOL_MODE_STATUS` for a given port to
    `REG_APE__TX_TO_NET_POOL_MODE_STATUS__ENABLE`. This sets up the block
    allocator which parcels out blocks of the write-only TX buffer region.

**Transmission.** To transmit a frame to the network, consider the following:

  - You will need to allocate enough blocks to store the frame via the block allocation
    procedure below.

  - Each block has a header. The first block of a frame has a bigger header.

  - Use the documentation of the block formats below to fill out a frame across as many
    blocks as necessary.

  - Once you've written the frame across one or more blocks, set the DOORBELL
    register, with the HEAD field set to the first block number of the frame,
    the TAIL field set to the last block number of the frame, and the LEN field
    set to the number of blocks. You're now done.

    A block is 128 bytes, so a block number is the offset from the start of the
    write-only region in units of 128 bytes.

The format for each block in a frame is as follows. Each line is a 32-bit word:

    0  Control
         bits  0: 6: Block Payload Length
         bits  7:??: Block Number of Subsequent Block (or 0 if last)
         bit     30: Set for first block    (guessed)
         bit     31: Set if NOT last block  (guessed)
    1  (Not written - leave uninitialized)

    IF FIRST BLOCK:
      2  Set to 0
      3  Set to number of bytes the frame is (not including FCS)
      4  Set to 0
      5  Set to 0
      6  Set to 0
      7  Set to 0
      8  Set to 0
      9  Set to number of blocks the frame needs
     10  (Not written - leave uninitialized)
     11  (Not written - leave uninitialized)
     12  (First word of actual payload, i.e. first word of Etherframe header, i.e. Dst MAC bytes 0-3)
     ... Payload continues
    ELSE:
      2  (First word of actual payload for this block)
      ... Payload continues

Due to the larger header of the first block, there's less room for frame data.

NOTE: Send frames without an FCS. The buffer length should be for an Ethernet
frame including header in bytes but without the FCS. Failure to observe this
rule may involve bizarre phenomena where ARP and ICMPv4 pass freely, but
TCP/UDPv4 are mysteriously eaten (unless the Ethertype is changed to something
other than IPv4, in which case they pass).

NOTE: Keep in mind Ethernet has a minimum frame length.

**Block allocation procedure.** Here's how to allocate a TX block. Ensure
you've done the initialization of the allocator already.

  - Set `REG_APE__TX_TO_NET_BUF_ALLOC` for the given port to REQ.
  - Poll the same register in a spin loop until `REG_APE__TX_TO_BUF_ALLOC__STATE` is `READY`.
  - Now your allocated block number is in `REG_APE__TX_TO_BUF_ALLOC__BLOCK`. This is in
    units of 128 bytes from the start of the write-only region for the given port
    (i.e. `0xA002_0000 + portNo*0x2000 + blockNo*128`).

**Errors.** Please note, if you get some of the frame length fields wrong this
will cause the hardware to raise an error. This raises `EXTINT_TX_ERROR`,
and upon examining `REG_APE__TX_STATE` you will find:

  - TXERR set, indicating that the error code field is valid (W2C), and
  - the error code field set to one of the values specified in otg.h.

This should never happen during normal (or exceptional) operation — it
indicates a programming error. Please *also* note that not *all* programming
errors in driving the TX To Network system result in one of these exceptions;
e.g. the TCP/UDP disappearance phenomenon noted above when using incorrect
buffer lengths.

### RX From Network

TODO

### TX To RMU (RGMII NC-SI)

The RGMII NC-SI peripheral is known as the "RMU". This is a variant of Gigabit
Ethernet's RGMII MAC-PHY interface, with a few small changes as specified by
the DMTF NC-SI specification. The APE core uses this peripheral to send and
receive Ethernet frames to/from a BMC. An interrupt is available for notification
of when a frame has been received (`EXTINT_RMU_EGRESS`).

**Initialization.** To setup the RMU so that it can be used, do this:

  - Enable the following bits in `REG_APE__MODE`:

    - `SWAP_{ATB,ARB}_DWORD`
    - `MEMORY_ECC`
    - `ICODE_PIP_RD_DISABLE`

  - Set `REG_APE__RMU_CONTROL` to `AUTO_DRV|RX|TX`. Also set bits 19 and 20
    (meaning unknown).

  - Set `REG_APE__BMC_NC_RX_CONTROL` to FLOW_CONTROL=1, HWM=0x240, XON_THRESHOLD=0x201F.

  - Set `REG_APE__NC_BMC_TX_CONTROL` to 0.

  - Set all eight `REG_APE__BMC_NC_RX_SRC_MAC_MATCHN_{HIGH,LOW}` to zero.

  - Set `REG_APE__ARB_CONTROL` as desired. Suggest PACKAGE_ID=0, TKNREL=0x14,
    START, and setting unknown bit 26 to 1.

**Transmission.** To transmit a frame to the BMC, do this:

  - The minimum frame size is 60 bytes (FCS is calculated automatically, don't include it).
    Pad it with zero words if necessary.

  - Wait for enough buffer space inside the RMU unit to be available.
    Spin until `REG_APE__NC_BMC_TX_STATUS__IN_FIFO` (which expresses the number
    of 32-bit words of spare room in the internal FIFO) is enough for the size
    of the frame you want to transmit.

  - Write every 32-bit word of the frame you want to transmit, *except the last word*,
    to `REG_APE__NC_BMC_TX_BUF_WRITE`. Every write to this register pushes a 32-bit
    word onto the FIFO. You may need to endian-swap each word.

  - Before writing the last word, write to `REG_APE__NC_BMC_TX_CONTROL` with
    LAST_BYTE_COUNT=the size of the frame in bytes, modulo 4. In other words,
    this specifies how many bytes of the last word written are part of the
    frame, as frames aren't necessarily a multiple of four bytes in length.

  - Finally, write the last word to `REG_APE__NC_BMC_TX_BUF_WRITE_LAST`.
    Writing to this instead of `REG_APE__NC_BMC_TX_BUF_WRITE` indicates that
    this is the last word in the frame.

### RX From RMU (RGMII NC-SI)

**Initialization.** See TX To RMU for the RMU init requirements.

**Reception.** To receive a frame from the BMC, do this:

  - Poll `REG_APE__BMC_NC_RX_STATUS` and ensure the `NEW` flag is set. If it isn't,
    there's no frame to receive; stop here.

  - The `REG_APE__BMC_NC_RX_STATUS__PACKET_LEN` field expresses the incoming
    frame length in bytes.
    
    Note: `REG_APE__BMC_NC_RX_STATUS__PASSTHRU` appears to indicate whether
    this is expected by the hardware to be a "pass-through" (that is,
    to-the-network) frame. Purpose of this field is unknown, since APE core can
    easily inspect frame itself to determine if it's an NCSI control packet or
    intended for passthrough, etc.

  - Read `REG_APE__BMC_NC_RX_BUF_READ` for each 32-bit word of the frame.
    Each read of this register pops one word of the incoming frame from the RX
    FIFO. You may need to endian-swap each word.

    **Please note** that it is essential that the correct number of reads be
    performed as the APE does not have any other way of communicating that it
    has finished consuming the frame.

  - Finally, read `REG_APE__BMC_NC_RX_BUF_READ` one more time. The value read
    can be discarded. This appears to just be a spacer FIFO pop which must
    happen after each frame RX.

Note that `EXTINT_RMU_EGRESS` is a **level-triggered** interrupt asserted
whenever there is data available for RX, so you will need to mask it when it
fires, then do the above to get the received frame.
