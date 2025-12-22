# MikanOS Development Container

このディレクトリには、MikanOSの開発環境をコンテナ化するための設定が含まれています。

## 必要なもの

- Docker
- Visual Studio Code
- Remote - Containers 拡張機能

## 使い方

### 1. コンテナで開く

1. VSCodeでこのプロジェクトを開く
2. 左下の緑色のアイコンをクリック
3. "Reopen in Container" を選択
4. 初回はイメージのビルドに時間がかかります（10-20分程度）

### 2. ビルドとテスト

コンテナ内のターミナルで以下を実行:

```bash
# UEFIローダーのビルド
./scripts/build_uefi.sh

# カーネルのビルド
./scripts/build_kernel.sh

# QEMUで実行
./scripts/run_qemu.sh
```

## コンテナ内の環境

- **ユーザー**: `vscode`
- **ホームディレクトリ**: `/home/vscode`
- **プロジェクトディレクトリ**: `/workspaces/mikanos_cpp`
- **EDK2**: `/home/vscode/edk2`
- **標準ライブラリ**: `/home/vscode/osbook/devenv`

## MikanLoaderPkgの配置

コンテナ起動時に、`postCreateCommand`で自動的にシンボリックリンクが作成されます:

```bash
/home/vscode/edk2/MikanLoaderPkg -> /workspaces/mikanos_cpp/MikanLoaderPkg
```

## X11転送（QEMU GUIの表示）

### Linux

ホストで以下を実行:

```bash
xhost +local:docker
```

### macOS

XQuartzをインストールして設定:

```bash
# XQuartzをインストール
brew install --cask xquartz

# XQuartzを起動して設定
# Preferences > Security > "Allow connections from network clients" をチェック

# X11転送を許可
xhost +localhost
```

### Windows

- VcXsrvまたはX410をインストール
- `DISPLAY`環境変数を適切に設定

## トラブルシューティング

### ビルドエラー: "MikanLoaderPkg not found"

シンボリックリンクを手動で作成:

```bash
ln -sf /workspaces/mikanos_cpp/MikanLoaderPkg /home/vscode/edk2/MikanLoaderPkg
```

### QEMU起動エラー: "Cannot open display"

X11転送の設定を確認してください。

### コンテナのリビルド

設定を変更した場合:

1. Ctrl+Shift+P
2. "Remote-Containers: Rebuild Container" を選択

## 含まれているツール

- clang-7, lld-7
- NASM
- QEMU
- EDK2（ビルド済み）
- MikanOS標準ライブラリ
- dosfstools, git, python3
