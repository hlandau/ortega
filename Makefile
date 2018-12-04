HOST_CC=clang
HOST_CFLAGS=-flto -O3 -g -Wno-undefined-internal -g
HOST_LD=$(HOST_CC)
HOST_LDFLAGS=-flto
#-Wl,--orphan-handling=error

USE_PROPRIETARY_APE ?=

ifneq ($(USE_PROPRIETARY_APE),)
APE_IMAGE_FN=$(USE_PROPRIETARY_APE)
else
ifneq ($(USE_REF_APE),)
APE_IMAGE_FN=ape_code_ref.bs.bin
else
APE_IMAGE_FN=ape_code_poc.bs.bin
endif
endif

ARM_CFLAGS=
TARGET_CFLAGS=-Wno-undefined-internal -DAPE_IMAGE_FN='"$(APE_IMAGE_FN)"'

all: otg.bin otg_dummy.bin otgdbg otgimg apeimg ape_shell.bin $(APE_IMAGE_FN)

clean:
	rm -f otg*.bin *.o *.s *.ll-opt *.ll-unopt *.bin.tmp* otgdbg otgimg s1stamp s2stamp apeimg apestamp

otg.bin: otg_stage1.ld otg_stage1.o otg_stage2.bin s1stamp otgimg
	ld.lld -o "$@.tmp" --oformat binary -T otg_stage1.ld otg_stage1.o
	./s1stamp "$@.tmp"
	./otgimg info "$@.tmp" 2>/dev/null | grep -E '^Defects:\s+none$$' >/dev/null
	mv "$@.tmp" "$@"
otg_stage1.o: otg_stage1.c otg.h otg_common.c otg_stage2.bin $(APE_IMAGE_FN)
	./cc_mips "$@" "$<" -DSTAGE1 $(TARGET_CFLAGS)

otg_stage2.bin: otg_stage2.ld otg_stage2.o s2stamp
	ld.lld -o "$@.tmp" --oformat binary -T otg_stage2.ld otg_stage2.o
	./s2stamp "$@.tmp"
	mv "$@.tmp" "$@"
otg_stage2.o: otg_stage2.c otg.h otg_common.c
	./cc_mips "$@" "$<" -DSTAGE2 $(TARGET_CFLAGS)

otg_dummy.bin: otg_stage1.ld otg_dummy.o s1stamp
	ld.lld -o "$@.tmp" --oformat binary -T otg_stage1.ld otg_dummy.o
	./s1stamp "$@.tmp"
	mv "$@.tmp" "$@"
otg_dummy.o: otg_dummy.c otg.h otg_common.c ape.bs.bin
	./cc_mips "$@" "$<" -DSTAGE1 $(TARGET_CFLAGS)

otgdbg: otgdbg.o
	$(HOST_LD) -g $(HOST_LDFLAGS) -o "$@" $^
otgdbg.o: otgdbg.c otg.h otg_common.c
	$(HOST_CC) -g -c $(HOST_CFLAGS) -o "$@" "$<" -DOTG_HOST

otgimg: otgimg.o
	$(HOST_LD) $(HOST_LDFLAGS) -o "$@" $^
otgimg.o: otgimg.c otg.h otg_common.c
	$(HOST_CC) -c $(HOST_CFLAGS) -o "$@" "$<" -DOTG_HOST

apeimg: apeimg.o
	$(HOST_LD) $(HOST_LDFLAGS) -o "$@" $^
apeimg.o: apeimg.c otg.h otg_common.c
	$(HOST_CC) -c $(HOST_CFLAGS) -g -o "$@" "$<" -DOTG_HOST

s1stamp: s1stamp.o
	$(HOST_CC) -flto -O3 -o "$@" $^
s1stamp.o: s1stamp.c otg.h otg_common.c
	$(HOST_CC) -c $(HOST_CFLAGS) -o "$@" "$<" -DOTG_HOST

s2stamp: s2stamp.o
	$(HOST_CC) -flto -O3 -o "$@" $^
s2stamp.o: s2stamp.c otg.h otg_common.c
	$(HOST_CC) -c $(HOST_CFLAGS) -o "$@" "$<" -DOTG_HOST

ape.bs.bin: ape.xbin byteswap
	cat "$<" | ./byteswap > "$@"

byteswap: byteswap.o
	$(HOST_CC) -flto -O3 -o "$@" $^
byteswap.o: byteswap.c otg.h otg_common.c
	$(HOST_CC) -c $(HOST_CFLAGS) -o "$@" "$<" -DOTG_HOST

ape_shell.bin: ape_shell.o ape_shell.ld
	ld.lld -o "$@" --oformat binary -T ape_shell.ld ape_shell.o
ape_shell.o: ape_shell.c
	./cc_arm "$@" "$<" -DAPE_SHELL $(ARM_CFLAGS)

apestamp: apestamp.o
	$(HOST_CC) -flto -O3 -o "$@" $^
apestamp.o: apestamp.c otg.h otg_common.c
	$(HOST_CC) -c $(HOST_CFLAGS) -o "$@" "$<" -DOTG_HOST

apebyteswap: apebyteswap.o
	$(HOST_CC) -flto -O3 -o "$@" $^
apebyteswap.o: apebyteswap.c otg.h otg_common.c
	$(HOST_CC) -c $(HOST_CFLAGS) -o "$@" "$<" -DOTG_HOST

ape_code_%.bs.bin: ape_code_%.bin apebyteswap
	cp "ape_code.bin" "$@.tmp"
	./apebyteswap "$@.tmp"
	mv "$@.tmp" "$@"
ape_code_%.bin: ape_code_%.o ape_code.ld apestamp apeimg
	ld.lld -o "$@.tmp" --oformat binary -T ape_code.ld "$<"
	./apestamp "$@.tmp" "$@.tmp2"
	./apeimg info "$@.tmp2" 2>/dev/null | grep -E '^Defects:\s+none$$' >/dev/null
	mv "$@.tmp2" "$@"
	rm "$@.tmp"
ape_code_%.o: ape_code_%.c
	./cc_arm "$@" "$<" -DOTG_APE $(ARM_CFLAGS)
