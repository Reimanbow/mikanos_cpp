#!/bin/bash
#
# 依存関係ファイル削除スクリプト
#
# 古い依存関係ファイル(.d)を削除します
# ビルド環境が変わったり、パスが変更された場合に実行してください
#

set -e  # エラーが発生したら即座に終了

# 色付き出力用
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# スクリプトのディレクトリ
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# プロジェクトルートディレクトリ
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# カーネルソースディレクトリ
KERNEL_DIR="$PROJECT_ROOT/kernel"

# 情報メッセージを表示
info() {
    echo -e "${BLUE}→ $1${NC}"
}

# 成功メッセージを表示
success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# 警告メッセージを表示
warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

echo "========================================="
echo "依存関係ファイル削除スクリプト"
echo "========================================="

# カーネルディレクトリの存在確認
if [ ! -d "$KERNEL_DIR" ]; then
    echo -e "${RED}エラー: カーネルディレクトリが見つかりません: $KERNEL_DIR${NC}" >&2
    exit 1
fi

# 依存関係ファイルを検索
info "依存関係ファイルを検索しています..."
DEPS_FILES=$(find "$KERNEL_DIR" -name ".*.d" -type f 2>/dev/null)

if [ -z "$DEPS_FILES" ]; then
    warning "依存関係ファイルが見つかりませんでした"
    echo "========================================="
    success "削除するファイルがありません"
    echo "========================================="
    exit 0
fi

# 見つかったファイル数を表示
DEPS_COUNT=$(echo "$DEPS_FILES" | wc -l)
info "見つかった依存関係ファイル: ${DEPS_COUNT}個"

# 確認メッセージ
echo ""
echo "以下のファイルを削除します:"
echo "$DEPS_FILES" | sed 's|^|  - |'
echo ""
echo -n "削除してよろしいですか? [y/N]: "
read -r response

case "$response" in
    [yY]|[yY][eE][sS])
        info "依存関係ファイルを削除しています..."
        find "$KERNEL_DIR" -name ".*.d" -type f -delete
        success "依存関係ファイルを削除しました"
        ;;
    *)
        warning "キャンセルされました"
        exit 0
        ;;
esac

echo ""
echo "========================================="
success "完了しました"
echo "========================================="
echo ""
info "次回のビルド時に依存関係ファイルが再生成されます"
