#!/bin/bash
#
# カーネルビルドスクリプト
#
# 使い方:
#   ./build_kernel.sh
#

set -e  # エラーが発生したら即座に終了

# 色付き出力用
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# スクリプトのディレクトリ（このスクリプトが配置されている場所）
# readlinkでシンボリックリンクの実体を取得
SCRIPT_PATH="$0"
if [ -L "$SCRIPT_PATH" ]; then
    SCRIPT_PATH="$(readlink -f "$SCRIPT_PATH")"
fi
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"

# プロジェクトルートディレクトリ
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# カーネルソースディレクトリ
KERNEL_DIR="$PROJECT_ROOT/kernel"

# 出力先ディレクトリ
OUTPUT_DIR="$PROJECT_ROOT/build"

# エラーメッセージを表示して終了
error_exit() {
    echo -e "${RED}エラー: $1${NC}" >&2
    exit 1
}

# 情報メッセージを表示
info() {
    echo -e "${BLUE}→ $1${NC}"
}

# 成功メッセージを表示
success() {
    echo -e "${GREEN}✓ $1${NC}"
}

echo "========================================="
echo "カーネルビルドスクリプト"
echo "========================================="

# カーネルディレクトリの存在確認
if [ ! -d "$KERNEL_DIR" ]; then
    error_exit "カーネルディレクトリが見つかりません: $KERNEL_DIR"
fi

# 出力ディレクトリの作成
info "出力ディレクトリの作成..."
mkdir -p "$OUTPUT_DIR"
success "出力ディレクトリ作成完了"

# 出力ディレクトリに移動してビルド（最終成果物を直接build/に作成）
cd "$OUTPUT_DIR" || error_exit "出力ディレクトリへの移動に失敗しました"

# ビルド実行
info "カーネルをビルドしています..."
echo "========================================="

# kernel/Makefileを使用してビルド
if [ -f "$KERNEL_DIR/Makefile" ]; then
    # Makefileがある場合はmakeを使用
    info "Makefileを使用してビルドします..."
    make -C "$KERNEL_DIR" clean
    make -C "$KERNEL_DIR"

    # ビルドされたkernel.elfをコピー
    if [ -f "$KERNEL_DIR/kernel.elf" ]; then
        cp "$KERNEL_DIR/kernel.elf" "kernel.elf" || error_exit "kernel.elfのコピーに失敗しました"
    else
        error_exit "kernel.elfが見つかりません"
    fi
else
    # Makefileがない場合は直接コンパイル
    info "main.cppをコンパイルしています..."
    clang++ \
        -I$HOME/osbook/devenv/x86_64-elf/include/c++/v1 \
        -I$HOME/osbook/devenv/x86_64-elf/include \
        -I$HOME/osbook/devenv/x86_64-elf/include/freetype2 \
        -I$HOME/edk2/MdePkg/Include \
        -I$HOME/edk2/MdePkg/Include/X64 \
        -nostdlibinc \
        -D__ELF__ \
        -D_LDBL_EQ_DBL \
        -D_GNU_SOURCE \
        -D_POSIX_TIMERS \
        -DEFIAPI='__attribute__((ms_abi))' \
        -O2 \
        --target=x86_64-elf \
        -fno-exceptions \
        -ffreestanding \
        -fno-rtti \
        -c "$KERNEL_DIR/main.cpp"

    info "kernel.elfをリンクしています..."
    ld.lld \
        -L$HOME/osbook/devenv/x86_64-elf/lib \
        --entry KernelMain \
        -z norelro \
        --image-base 0x100000 \
        --static \
        -o kernel.elf \
        main.o
fi

success "カーネルビルド完了"

# kernel.elfのサイズを表示
if [ -f "kernel.elf" ]; then
    KERNEL_SIZE=$(stat -c%s "kernel.elf")
    info "kernel.elfのサイズ: $KERNEL_SIZE バイト"
    success "出力先: $(pwd)/kernel.elf"
else
    error_exit "kernel.elfが見つかりません"
fi

# 元のディレクトリに戻る
cd - > /dev/null

# 完了メッセージ
echo ""
echo "========================================="
success "カーネルビルドが完了しました！"
echo "========================================="
