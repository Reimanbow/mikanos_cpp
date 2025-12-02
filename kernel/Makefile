# カーネルビルド用Makefile

TARGET = kernel.elf
OBJS = main.o

# コンパイラとリンカの設定
CXX = clang++
LD = ld.lld

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

CXXFLAGS = -O2 \
           --target=x86_64-elf \
           -fno-exceptions \
           -ffreestanding \
           -fno-rtti \
           -Wall \
           -Wextra

# リンカフラグ
# 環境変数のLDFLAGS（ライブラリパス）を使用
LDFLAGS += --entry KernelMain \
           -z norelro \
           --image-base 0x100000 \
           --static

# ビルドルール
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

# デバッグ用: 変数の内容を表示
debug:
	@echo "TARGET: $(TARGET)"
	@echo "OBJS: $(OBJS)"
	@echo "CPPFLAGS: $(CPPFLAGS)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
