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

# Loader.efiのパス
LOADER_EFI="build/Loader.efi"

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

# ファイルサイズの表示
FILE_SIZE=$(stat -c%s "$LOADER_EFI")
info "ファイルサイズ: $FILE_SIZE バイト"

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
cd build || error_exit "buildディレクトリへの移動に失敗しました"

# 絶対パスでrun_qemu.shを実行
"$RUN_QEMU_SCRIPT" Loader.efi

# 元のディレクトリに戻る
cd - > /dev/null

echo ""
success "QEMUを終了しました"
