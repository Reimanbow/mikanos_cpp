#!/bin/bash
#
# UEFIアプリケーションをUSBメモリに書き込むスクリプト
#
# 使い方:
#   ./write_usb.sh [デバイス名]
#   例: ./write_usb.sh /dev/sda
#
# デバイス名を指定しない場合は対話的にデバイスを選択できます

set -e  # エラーが発生したら即座に終了

# 色付き出力用
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ビルド済みのLoaderファイルパス
LOADER_FILE="build/Loader.efi"

# マウントポイント
MOUNT_POINT="/mnt/usbmem"

# エラーメッセージを表示して終了
error_exit() {
    echo -e "${RED}エラー: $1${NC}" >&2
    exit 1
}

# 警告メッセージを表示
warning() {
    echo -e "${YELLOW}警告: $1${NC}"
}

# 成功メッセージを表示
success() {
    echo -e "${GREEN}$1${NC}"
}

# root権限チェック
if [[ $EUID -ne 0 ]]; then
   error_exit "このスクリプトはroot権限で実行する必要があります (sudo を使用してください)"
fi

# Loaderファイルの存在確認
if [ ! -f "$LOADER_FILE" ]; then
    error_exit "Loader.efi が見つかりません: $LOADER_FILE"
fi

# デバイスの選択
if [ -z "$1" ]; then
    echo "========================================="
    echo "接続されているブロックデバイス一覧:"
    echo "========================================="
    lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,MODEL
    echo ""
    warning "注意: 間違ったデバイスを選択するとデータが消失します！"
    echo ""
    read -p "書き込み先のデバイスを入力してください (例: /dev/sdb): " DEVICE
else
    DEVICE="$1"
fi

# デバイス名の検証
if [ ! -b "$DEVICE" ]; then
    error_exit "$DEVICE はブロックデバイスではありません"
fi

# デバイス情報の表示
echo ""
echo "========================================="
echo "選択されたデバイス情報:"
echo "========================================="
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,MODEL "$DEVICE" || error_exit "デバイス情報の取得に失敗しました"
echo ""

# 最終確認
warning "警告: $DEVICE の全データが削除されます！"
read -p "本当に続行しますか? (yes と入力してください): " CONFIRM

if [ "$CONFIRM" != "yes" ]; then
    echo "処理をキャンセルしました"
    exit 0
fi

echo ""
echo "========================================="
echo "USBメモリへの書き込みを開始します"
echo "========================================="

# ステップ1: アンマウント
echo "[1/7] デバイスのアンマウント..."
umount "$DEVICE"* 2>/dev/null || true  # エラーを無視（既にアンマウントされている場合）

# ステップ2: FATファイルシステムの作成
echo "[2/7] FAT32ファイルシステムの作成..."
mkfs.fat -F 32 "$DEVICE" || error_exit "ファイルシステムの作成に失敗しました"

# ステップ3: マウントポイントの作成
echo "[3/7] マウントポイントの作成..."
mkdir -p "$MOUNT_POINT"

# ステップ4: デバイスのマウント
echo "[4/7] デバイスのマウント..."
mount "$DEVICE" "$MOUNT_POINT" || error_exit "マウントに失敗しました"

# ステップ5: EFIディレクトリの作成
echo "[5/7] EFI/BOOTディレクトリの作成..."
mkdir -p "$MOUNT_POINT/EFI/BOOT"

# ステップ6: Loaderのコピー
echo "[6/7] Loader.efiをBOOTX64.EFIとしてコピー..."
cp "$LOADER_FILE" "$MOUNT_POINT/EFI/BOOT/BOOTX64.EFI" || error_exit "ファイルのコピーに失敗しました"

# ファイルが正しくコピーされたか確認
if [ -f "$MOUNT_POINT/EFI/BOOT/BOOTX64.EFI" ]; then
    FILE_SIZE=$(stat -c%s "$MOUNT_POINT/EFI/BOOT/BOOTX64.EFI")
    echo "   -> コピー完了 (サイズ: $FILE_SIZE バイト)"
else
    error_exit "ファイルのコピー確認に失敗しました"
fi

# ステップ7: アンマウント
echo "[7/7] アンマウント..."
sync  # バッファをフラッシュ
umount "$MOUNT_POINT" || error_exit "アンマウントに失敗しました"

echo ""
success "========================================="
success "書き込みが完了しました！"
success "========================================="
echo ""
echo "USBメモリを安全に取り外すことができます。"
echo "PCを再起動してUEFIブートメニューからUSBメモリを選択してください。"
