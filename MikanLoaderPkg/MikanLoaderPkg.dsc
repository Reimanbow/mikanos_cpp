## @file
# MikanLoaderPkgプラットフォーム記述ファイル (.dsc)
# このファイルはビルド設定を定義します
# どのモジュールをビルドするか、どのライブラリを使用するかなどを指定します
##

[Defines]
  PLATFORM_NAME                  = MikanLoaderPkg                          # プラットフォーム名
  PLATFORM_GUID                  = d3f11f4e-71e9-11e8-a7e1-33fd4f7d5a3e   # プラットフォームを一意に識別するGUID
  PLATFORM_VERSION               = 0.1                                     # プラットフォームのバージョン
  DSC_SPECIFICATION              = 0x00010005                              # DSCファイルの仕様バージョン
  OUTPUT_DIRECTORY               = Build/MikanLoader$(ARCH)                # ビルド成果物の出力先ディレクトリ
  SUPPORTED_ARCHITECTURES        = X64                                     # サポートするアーキテクチャ（64ビットx86）
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT                     # ビルドターゲット（デバッグ/リリース/最適化なし）

# ライブラリクラスのマッピング
# 各ライブラリクラス名と、実際に使用するライブラリの実装ファイル(.inf)を対応付けます
[LibraryClasses]
  # UEFIアプリケーションの基本ライブラリ
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf  # エントリーポイント処理
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf                                                         # UEFI基本機能（Print等）

  # 基礎的なライブラリ群
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf                                    # 基本的な演算・文字列処理
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf                  # メモリ操作（コピー、比較等）
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf                 # デバッグ出力（Null実装=出力なし）
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf         # デバイスパス操作
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf  # メモリ確保・解放
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf                       # プラットフォーム設定データベース
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf                         # 文字列フォーマット・出力
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf          # UEFIブートサービス
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf  # UEFIランタイムサービス

# ビルド対象のコンポーネント（モジュール）一覧
[Components]
  MikanLoaderPkg/Loader.inf  # Loaderモジュールをビルド対象に含める