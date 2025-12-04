# カーネルビルド用Makefile

TARGET = kernel.elf
OBJS = main.o graphics.o font.o
DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

# コンパイルフラグ
CPPFLAGS = -I$(HOME)/osbook/devenv/x86_64-elf/include/c++/v1 \
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
LDFLAGS  += --entry KernelMain -z norelro --image-base 0x100000 --static

# ビルドルール
.PHONY: all clean

all: $(TARGET)

.PHONY: clean
clean:
	rm -rf *.o

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS)

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $<
	@clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $(patsubst %.o,.%.d,$@)
	@sed --in-place 's|$(notdir $@)|$@|' $(patsubst %.o,.%.d,$@)

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)