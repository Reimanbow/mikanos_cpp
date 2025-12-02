/**
 * @file Main.c
 * 
 * MikanLoaderのメインプログラム
 * UEFIアプリケーションとして起動し、メッセージを表示する
 */

#include <Uefi.h>
#include <Library/UefiLib.h>

/**
 * UEFIアプリケーションのエントリーポイント
 *
 * @param image_handle このアプリケーションのイメージハンドル
 * @param system_table UEFIシステムテーブルへのポインタ
 * @return EFI_SUCCESS 正常終了
 */
EFI_STATUS EFIAPI UefiMain(
		EFI_HANDLE image_handle,
		EFI_SYSTEM_TABLE *system_table) {
	// "Hello, Mikan World!" と画面に表示
	Print(L"Hello, Mikan World!\n");

	// 無限ループでプログラムを停止（デバッグ・確認用）
	while (1);

	return EFI_SUCCESS;
}