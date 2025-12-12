# カーネルビルド用Makefile

TARGET = kernel.elf
OBJS = main.o graphics.o mouse.o font.o hankaku.o newlib_support.o console.o \
       pci.o asmfunc.o libcxx_support.o logger.o interrupt.o segment.o paging.o memory_manager.o \
	   window.o layer.o \
       usb/memory.o usb/device.o usb/xhci/ring.o usb/xhci/trb.o usb/xhci/xhci.o \
       usb/xhci/port.o usb/xhci/device.o usb/xhci/devmgr.o usb/xhci/registers.o \
       usb/classdriver/base.o usb/classdriver/hid.o usb/classdriver/keyboard.o \
       usb/classdriver/mouse.o
DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

# コンパイルフラグ
CFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone

CPPFLAGS = -I. \
		   -I$(HOME)/osbook/devenv/x86_64-elf/include/c++/v1 \
           -I$(HOME)/osbook/devenv/x86_64-elf/include \
           -I$(HOME)/osbook/devenv/x86_64-elf/include/freetype2 \
           -I$(HOME)/edk2/MdePkg/Include \
           -I$(HOME)/edk2/MdePkg/Include/X64 \
           -nostdlibinc \
           -D__ELF__ \
           -D_LDBL_EQ_DBL \
           -D_GNU_SOURCE \
           -D_POSIX_TIMERS \
           -DEFIAPI='__attribute__((ms_abi))'

CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
            -fno-exceptions -fno-rtti -std=c++17

# リンカフラグ
# 環境変数のLDFLAGS（ライブラリパス）を使用
LDFLAGS  += --entry KernelMain -z norelro --image-base 0x100000 --static \
            -L$(HOME)/osbook/devenv/x86_64-elf/lib

# ビルドルール
.PHONY: all clean

all: $(TARGET)

.PHONY: clean
clean:
	rm -rf *.o

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS) -lc -lc++ -lc++abi

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $(patsubst %.o,.%.d,$@)
	@sed --in-place 's|$(notdir $@)|$@|' $(patsubst %.o,.%.d,$@)

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $< -o $@
	@clang $(CPPFLAGS) $(CFLAGS) -MM $< > $(patsubst %.o,.%.d,$@)
	@sed --in-place 's|$(notdir $@)|$@|' $(patsubst %.o,.%.d,$@)

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.asm Makefile
	nasm -f elf64 -o $@ $<

hankaku.bin: hankaku.txt
	../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

.%.d: %.bin
	touch $@

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)