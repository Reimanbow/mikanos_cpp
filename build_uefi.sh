#!/bin/bash
#
# MikanLoaderをビルドしてLoader.efiをコピーするスクリプト
#
# 使い方:
#   ./build.sh [ビルドターゲット]
#   例: ./build.sh DEBUG    (デバッグビルド・デフォルト)
#       ./build.sh RELEASE  (リリースビルド)
#

set -e  # エラーが発生したら即座に終了

# 色付き出力用
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# EDK2のディレクトリ
EDK2_DIR="$HOME/edk2"

# 出力先ディレクトリ
OUTPUT_DIR="$HOME/mikan_neo/build"

# ビルドターゲット（デフォルトはDEBUG）
BUILD_TARGET="${1:-DEBUG}"

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
    echo -e "${GREEN}✓ $1${NC}"
}

# 情報メッセージを表示
info() {
    echo -e "${BLUE}→ $1${NC}"
}

# ビルドターゲットの検証
case "$BUILD_TARGET" in
    DEBUG|RELEASE|NOOPT)
        ;;
    *)
        error_exit "無効なビルドターゲット: $BUILD_TARGET (DEBUG, RELEASE, NOOPT のいずれかを指定してください)"
        ;;
esac

echo "========================================="
echo "MikanLoader ビルドスクリプト"
echo "========================================="
info "ビルドターゲット: $BUILD_TARGET"
info "EDK2ディレクトリ: $EDK2_DIR"
info "出力先: $OUTPUT_DIR"
echo ""

# EDK2ディレクトリの存在確認
if [ ! -d "$EDK2_DIR" ]; then
    error_exit "EDK2ディレクトリが見つかりません: $EDK2_DIR"
fi

# MikanLoaderPkgの存在確認
if [ ! -d "$EDK2_DIR/MikanLoaderPkg" ]; then
    error_exit "MikanLoaderPkgが見つかりません: $EDK2_DIR/MikanLoaderPkg"
fi

# 出力ディレクトリの作成
info "出力ディレクトリの作成..."
mkdir -p "$OUTPUT_DIR" || error_exit "出力ディレクトリの作成に失敗しました"
success "出力ディレクトリ作成完了"

# EDK2ディレクトリに移動
cd "$EDK2_DIR" || error_exit "EDK2ディレクトリへの移動に失敗しました"

# EDK2環境のセットアップ
info "EDK2環境のセットアップ..."
if [ ! -f "edksetup.sh" ]; then
    error_exit "edksetup.sh が見つかりません"
fi

# BaseToolsの存在確認
if [ ! -d "BaseTools" ]; then
    error_exit "BaseTools が見つかりません"
fi

# EDK2環境変数をセットアップ（サブシェルではなく現在のシェルで実行）
source edksetup.sh || error_exit "EDK2環境のセットアップに失敗しました"
success "EDK2環境のセットアップ完了"

# ビルド実行
echo ""
info "ビルドを開始します..."
echo "========================================="

# buildコマンドを実行
# -p: プラットフォームDSCファイル
# -a: アーキテクチャ
# -t: ツールチェイン
# -b: ビルドターゲット
if build -p MikanLoaderPkg/MikanLoaderPkg.dsc -a X64 -t CLANG38 -b "$BUILD_TARGET"; then
    echo "========================================="
    success "ビルド成功"
else
    echo "========================================="
    error_exit "ビルドに失敗しました"
fi

# ビルド成果物のパス
SOURCE_EFI="Build/MikanLoaderX64/${BUILD_TARGET}_CLANG38/X64/Loader.efi"

# ビルド成果物の存在確認
if [ ! -f "$SOURCE_EFI" ]; then
    error_exit "Loader.efi が見つかりません: $SOURCE_EFI"
fi

# ファイルサイズの取得
FILE_SIZE=$(stat -c%s "$SOURCE_EFI")
info "Loader.efi のサイズ: $FILE_SIZE バイト"

# Loader.efiのコピー
echo ""
info "Loader.efi をコピーしています..."
cp "$SOURCE_EFI" "$OUTPUT_DIR/Loader.efi" || error_exit "ファイルのコピーに失敗しました"
success "コピー完了: $OUTPUT_DIR/Loader.efi"

# 最終確認
if [ -f "$OUTPUT_DIR/Loader.efi" ]; then
    COPIED_SIZE=$(stat -c%s "$OUTPUT_DIR/Loader.efi")
    if [ "$FILE_SIZE" -eq "$COPIED_SIZE" ]; then
        success "ファイルの整合性を確認しました"
    else
        warning "ファイルサイズが一致しません"
    fi
fi

# 完了メッセージ
echo ""
echo "========================================="
success "全ての処理が完了しました！"
echo "========================================="
