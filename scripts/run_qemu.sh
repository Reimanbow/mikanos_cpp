#!/bin/bash
#
# QEMUでMikanLoaderを実行するスクリプト
#
# 使い方:
#   ./run_qemu.sh
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

# Loader.efiのパス
LOADER_EFI="$PROJECT_ROOT/build/Loader.efi"

# kernel.elfのパス
KERNEL_ELF="$PROJECT_ROOT/build/kernel.elf"

# run_qemu.shのパス（osbook/devenvを使用）
RUN_QEMU_SCRIPT="$HOME/osbook/devenv/run_qemu.sh"

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
echo "QEMU実行スクリプト"
echo "========================================="

# Loader.efiの存在確認
if [ ! -f "$LOADER_EFI" ]; then
    error_exit "Loader.efi が見つかりません: $LOADER_EFI

まずビルドを実行してください:
  ./build_uefi.sh"
fi

info "Loader.efi: $LOADER_EFI"

# Loader.efiのファイルサイズの表示
LOADER_SIZE=$(stat -c%s "$LOADER_EFI")
info "Loader.efiのサイズ: $LOADER_SIZE バイト"

# kernel.elfの存在確認
if [ ! -f "$KERNEL_ELF" ]; then
    error_exit "kernel.elf が見つかりません: $KERNEL_ELF

カーネルをビルドしてください"
fi

info "kernel.elf: $KERNEL_ELF"

# kernel.elfのファイルサイズの表示
KERNEL_SIZE=$(stat -c%s "$KERNEL_ELF")
info "kernel.elfのサイズ: $KERNEL_SIZE バイト"

# run_qemu.shの存在確認
if [ ! -f "$RUN_QEMU_SCRIPT" ]; then
    error_exit "run_qemu.sh が見つかりません: $RUN_QEMU_SCRIPT

devenvディレクトリが正しい場所にあるか確認してください"
fi

# run_qemu.shの実行
echo ""
info "QEMUを起動します..."
echo "========================================="
echo ""

# buildディレクトリに移動してrun_qemu.shを実行
cd "$PROJECT_ROOT/build" || error_exit "buildディレクトリへの移動に失敗しました"

# 絶対パスでrun_qemu.shを実行（Loader.efiとkernel.elfの両方を引数として渡す）
"$RUN_QEMU_SCRIPT" Loader.efi kernel.elf

# 元のディレクトリに戻る
cd - > /dev/null

echo ""
success "QEMUを終了しました"
