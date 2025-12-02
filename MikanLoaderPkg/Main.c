/**
 * @file Main.c
 *
 * MikanLoaderのメインプログラム
 * UEFIアプリケーションとして起動し、メッセージを表示する
 */

#include <Uefi.h> 								// UEFI基本定義（EFI_STATUS, EFI_HANDLEなどの型定義）
#include <Library/UefiLib.h> 					// UEFI基本ライブラリ（Print関数など）
#include <Library/UefiBootServicesTableLib.h> 	// UEFIブートサービステーブルへのアクセス（gBS グローバル変数）
#include <Library/PrintLib.h> 					// 文字列フォーマット・出力ライブラリ
#include <Protocol/LoadedImage.h> 				// 実行中のUEFIアプリケーション自身の情報（ロード元、メモリ位置など）を取得できる
#include <Protocol/SimpleFileSystem.h> 			// ファイルの読み書きを行うための標準インターフェース
#include <Protocol/DiskIo2.h> 					// ブロックデバイスへの直接的な読み書き
#include <Protocol/BlockIo.h> 					// ディスクなどのブロックデバイスへの低レベルアクセス
#include <Guid/FileInfo.h>

/**
 * @brief メモリマップ情報を格納する構造体
 *
 * UEFIファームウェアが管理しているメモリマップ（どのメモリ領域が何に使われているか）を取得・保持するためのデータ構造
 * OSを起動する際には、このメモリマップ情報をOSに渡す必要がある
 */
struct MemoryMap {
	UINTN	buffer_size;        // メモリマップを格納するバッファのサイズ（バイト）
	VOID*	buffer;             // メモリマップデータを格納するバッファへのポインタ
	UINTN	map_size;           // 実際のメモリマップのサイズ（GetMemoryMap実行後に設定される）
	UINTN	map_key;            // メモリマップの状態を識別するキー（ExitBootServices時に必要）
	UINTN	descriptor_size;    // 各メモリディスクリプタ（1エントリ）のバイト数
	UINT32	descriptor_version; // メモリディスクリプタ構造体のバージョン
};

/**
 * @brief 現在のメモリマップを取得する関数
 *
 * UEFI Boot Servicesが管理しているメモリマップ（物理メモリの使用状況）を取得する
 * メモリマップには以下の情報が含まれる：
 * - どのメモリ領域がUEFIファームウェアに使われているか
 * - どこが空き領域か
 * - どこがハードウェア予約領域か
 * など
 *
 * @param map メモリマップ情報を格納する構造体へのポインタ
 *            呼び出し前にbufferとbuffer_sizeを設定しておく必要がある
 *
 * @return EFI_SUCCESS          正常に取得できた
 * @return EFI_BUFFER_TOO_SMALL バッファサイズが不足（map->map_sizeに必要なサイズが設定される）
 * @return その他                エラーコード
 */
EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
	// バッファが確保されていない場合はエラーを返す
	if (map->buffer == NULL) {
		return EFI_BUFFER_TOO_SMALL;
	}

	// map_sizeにバッファサイズを設定（GetMemoryMapの入出力パラメータ）
	map->map_size = map->buffer_size;

	/**
	 * gBS->GetMemoryMap() - UEFIのメモリマップ取得関数
	 * 
	 * 引数の説明:
	 * - MemoryMapSize (IN/OUT):
	 *     [入力] メモリマップ書き込み用バッファのサイズ
	 *     [出力] 実際のメモリマップのサイズ（必要なサイズ）
	 *
	 * - MemoryMap (OUT):
	 *     メモリマップを書き込むバッファの先頭ポインタ
	 *     EFI_MEMORY_DESCRIPTOR構造体の配列として格納される
	 *
	 * - MapKey (OUT):
	 *     メモリマップを識別するための値
	 *     ExitBootServices()実行時にこの値を渡す必要がある
	 *     （メモリマップが変更されていないことを確認するため）
	 *
	 * - DescriptorSize (OUT):
	 *     メモリディスクリプタ1個のバイト数
	 *     配列を走査する際に使用する
	 *
	 * - DescriptorVersion (OUT):
	 *     メモリディスクリプタ構造体のバージョン番号
	 */
	return gBS->GetMemoryMap(
		&map->map_size,
		(EFI_MEMORY_DESCRIPTOR*)map->buffer,
		&map->map_key,
		&map->descriptor_size,
		&map->descriptor_version);
}

/**
 * @brief メモリタイプの数値を人間が読める文字列に変換する
 *
 * EFI_MEMORY_TYPEは内部的には数値（enum）で表現されているが、
 * デバッグやログ出力時にわかりやすくするため、対応する名前の文字列を返す。
 *
 * @param type メモリタイプ（EFI_MEMORY_TYPE列挙型）
 * @return メモリタイプ名を表すUnicode文字列
 *
 * 主なメモリタイプの意味:
 * - EfiConventionalMemory: OSが自由に使える通常のメモリ（最重要）
 * - EfiBootServicesCode/Data: UEFIブートサービスが使用中（OS起動後は解放可能）
 * - EfiLoaderCode/Data: ブートローダーが使用中
 * - EfiRuntimeServicesCode/Data: OS起動後もUEFIが使用（触ってはいけない）
 * - EfiReservedMemoryType: 予約済み（使用不可）
 * - EfiMemoryMappedIO: デバイスのメモリ領域（通常のメモリではない）
 * - EfiACPIReclaimMemory: ACPI用（ACPI読み込み後は解放可能）
 */
const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";          // 予約済み領域
    case EfiLoaderCode: return L"EfiLoaderCode";                          // ローダーのコード
    case EfiLoaderData: return L"EfiLoaderData";                          // ローダーのデータ
    case EfiBootServicesCode: return L"EfiBootServicesCode";              // UEFIブートサービスコード
    case EfiBootServicesData: return L"EfiBootServicesData";              // UEFIブートサービスデータ
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";        // UEFIランタイムコード
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";        // UEFIランタイムデータ
    case EfiConventionalMemory: return L"EfiConventionalMemory";          // 通常の使用可能メモリ
    case EfiUnusableMemory: return L"EfiUnusableMemory";                  // 使用不可メモリ（故障等）
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";            // ACPI用（後で回収可能）
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";                    // ACPI NVS（不揮発性）
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";                  // メモリマップドI/O
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace"; // I/Oポート空間
    case EfiPalCode: return L"EfiPalCode";                                // PALコード（Itanium用）
    case EfiPersistentMemory: return L"EfiPersistentMemory";              // 永続メモリ（NVDIMM等）
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";                    // 最大値（境界チェック用）
    default: return L"InvalidMemoryType";                                  // 不明な型
  }
}

/**
 * @brief メモリマップをCSV形式でファイルに保存する
 *
 * 取得したメモリマップを人間が読みやすいCSV形式でファイルに出力する。
 * OSカーネルにメモリマップを渡す前に、デバッグ用として内容を確認できる。
 *
 * @param map 保存するメモリマップ情報（GetMemoryMapで取得済み）
 * @param file 書き込み先のファイル（既に開いている必要がある）
 * @return EFI_SUCCESS 正常終了
 *
 * 出力形式（CSV）:
 * Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute
 * 0, 7, EfiConventionalMemory, 00001000, 9F, F
 * 1, 4, EfiBootServicesData, 000A0000, 60, F
 * ...
 *
 * 各カラムの意味:
 * - Index: エントリ番号（0から始まる）
 * - Type: メモリタイプの数値
 * - Type(name): メモリタイプの名前（GetMemoryTypeUnicodeで変換）
 * - PhysicalStart: 物理アドレスの開始位置（16進数）
 * - NumberOfPages: ページ数（1ページ = 4KB）
 * - Attribute: メモリ属性フラグ（キャッシュ可能、書き込み可能など）
 */
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file) {
	CHAR8 buf[256];  // 1行分の出力バッファ
	UINTN len;

	// CSVヘッダーを書き込む
	CHAR8* header =
		"Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
	len = AsciiStrLen(header);
	file->Write(file, &len, header);

	// デバッグ情報: メモリマップのバッファアドレスとサイズを画面に表示
	Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
		map->buffer, map->map_size);

	// メモリマップの各エントリを走査して出力
	// 注意: メモリディスクリプタは可変長なので、descriptor_sizeずつ進める
	EFI_PHYSICAL_ADDRESS iter;
	int i;
	for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
		 iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
		 iter += map->descriptor_size, i++) {  // descriptor_sizeずつポインタを進める

		// 現在のエントリを取得
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;

		// 1行分のCSVデータを整形
		// %u: エントリ番号, %x: タイプ（16進）, %-ls: タイプ名（左寄せ）,
		// %08lx: 物理アドレス（8桁16進）, %lx: ページ数（16進）
		len = AsciiSPrint(
			buf, sizeof(buf),
			"%u, %x, %-ls, %08lx, %lx, %lx\n",
			i, desc->Type, GetMemoryTypeUnicode(desc->Type),
			desc->PhysicalStart, desc->NumberOfPages,
			desc->Attribute & 0xfffffflu);  // 下位24ビットのみ使用

		// ファイルに書き込む
		file->Write(file, &len, buf);
	}

	return EFI_SUCCESS;
}

/**
 * @brief このアプリケーションが起動されたデバイスのルートディレクトリを開く
 *
 * この関数は以下の手順でルートディレクトリを取得する：
 * 1. image_handleからLoadedImageプロトコルを取得
 * 2. LoadedImageプロトコルから起動元デバイスのハンドルを取得
 * 3. デバイスハンドルからSimpleFileSystemプロトコルを取得
 * 4. SimpleFileSystemプロトコルを使ってルートディレクトリを開く
 *
 * これにより、このローダーと同じディレクトリやドライブにあるファイル（例: カーネルファイル）を読み込むことができるようになる。
 *
 * @param image_handle このアプリケーションのイメージハンドル（UefiMainの第1引数）
 * @param root 出力パラメータ: 開いたルートディレクトリのファイルプロトコルポインタ
 * @return EFI_SUCCESS 正常終了
 */
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
	EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

	// ステップ1: LoadedImageプロトコルを取得
	// このアプリケーション自身の情報（どこからロードされたかなど）を取得
	gBS->OpenProtocol(
		image_handle,                          // このアプリのハンドル
		&gEfiLoadedImageProtocolGuid,          // 取得したいプロトコルのGUID
		(VOID**)&loaded_image,                 // 出力先
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

	// ステップ2-3: デバイスハンドルからSimpleFileSystemプロトコルを取得
	// loaded_image->DeviceHandle は、このアプリがロードされたデバイス（USBメモリやHDDなど）を指す
	gBS->OpenProtocol(
		loaded_image->DeviceHandle,            // 起動元デバイスのハンドル
		&gEfiSimpleFileSystemProtocolGuid,     // ファイルシステムプロトコルのGUID
		(VOID**)&fs,                           // 出力先
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

	// ステップ4: ファイルシステムのルートディレクトリを開く
	// これでファイルの読み書きができるようになる
	fs->OpenVolume(fs, root);

	return EFI_SUCCESS;
}

/**
 * @brief UEFIアプリケーションのエントリーポイント
 *
 * UEFIファームウェアがこのアプリケーションを起動した際に、最初に呼び出される関数。
 * C言語のmain関数に相当する。
 *
 * @param image_handle このアプリケーション自身を識別するハンドル（ID）
 *                     主な用途:
 *                     - 自分がどのデバイスから起動されたかを調べる
 *                     - 起動元デバイスのファイルを読み込む（例: カーネルファイル）
 *                     - 自分自身の情報（メモリ位置、サイズなど）を取得する
 *                     例: このローダーと同じディレクトリにあるカーネルファイルを読み込む際に、image_handleから起動デバイスを特定する
 * @param system_table UEFI全体の機能にアクセスするための入口
 *                     主な用途:
 *                     - メモリの確保・解放（BootServices）
 *                     - 画面への出力（ConOut）
 *                     - キーボード入力の取得（ConIn）
 *                     - 時刻の取得（RuntimeServices）
 *                     実際にはgBS（グローバル変数）やPrint()関数など、より使いやすいラッパー経由で利用することが多い
 * @return EFI_SUCCESS 正常終了
 */
EFI_STATUS EFIAPI UefiMain(
		EFI_HANDLE image_handle,
		EFI_SYSTEM_TABLE *system_table) {
	// 起動メッセージを表示
	Print(L"Hello, Mikan World!\n");

	// メモリマップを格納するバッファを確保（16KB = 4096 * 4バイト）
	CHAR8 memmap_buf[4096 * 4];

	// MemoryMap構造体を初期化
	// {バッファサイズ, バッファポインタ, マップサイズ, マップキー, ディスクリプタサイズ, バージョン}
	struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};

	// UEFIからメモリマップを取得
	GetMemoryMap(&memmap);

	// このローダーが起動されたデバイス（USBメモリやHDDなど）のルートディレクトリを開く
	EFI_FILE_PROTOCOL* root_dir;
	OpenRootDir(image_handle, &root_dir);

	// ルートディレクトリに "memmap" という名前でファイルを作成
	// EFI_FILE_MODE_READ | WRITE | CREATE: 読み書き可能、存在しない場合は新規作成
	EFI_FILE_PROTOCOL* memmap_file;
	root_dir->Open(
		root_dir, &memmap_file, L"\\memmap",
		EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

	// メモリマップをCSV形式でファイルに書き込む
	SaveMemoryMap(&memmap, memmap_file);

	// ファイルを閉じる（書き込みを確定）
	memmap_file->Close(memmap_file);

	// ルートにあるkernel.elfという名前のファイルを読み込み専用で開く
	EFI_FILE_PROTOCOL* kernel_file;
	root_dir->Open(
		root_dir, &kernel_file, L"\\kernel.elf",
		EFI_FILE_MODE_READ, 0);

	/**
	 * ファイル全体を読み込むためのメモリを確保する
	 * そのためにファイルの大きさを知る必要があるので、kernel_file->GetInfo()でファイル情報を取得する
	 * EFI_FILE_INFO型の最後のメンバFileNameは配列の要素数が不定であるため、
	 * sizeof(EFI_FILE_INFO)はメンバAttributeまでの大きさと等しくなる
	 * \kernel.elfという12文字を格納するため、EFI_FILE_INFOの大きさよりsizeof(CHAR16)*12バイトだけ大きい領域を確保する
	 */
	UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
	UINT8 file_info_buffer[file_info_size];
	kernel_file->GetInfo(
		kernel_file, &gEfiFileInfoGuid,
		&file_info_size, file_info_buffer);

	// EFI_FILE_INFO型のfile_info_bufferからFileSizeを読み取る
	EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
	UINTN kernel_file_size = file_info->FileSize;

	/**
	 * gBS->AllocatePages()を使ってファイルを格納できる十分な大きさのメモリ領域を確保する
	 * 第1引数: メモリの確保の仕方
	 * 	- AllocateAnyPages: 空いている場所に確保する
	 * 	- AllocateMaxAddress: 指定したアドレス以下で空いている場所に確保する
	 * 	- AllocateAddress: 指定したアドレスに確保する
	 * 第2引数: 確保するメモリ領域の種別
	 * 第3引数: 大きさ
	 * 第4引数: 確保したメモリ領域のアドレスを書き込む変数
	 * 
	 * 第1引数はカーネルファイルは0x100000番地に配置して動作させる(ld.lldのオプション--image-baseで指定)ためAllocateAddresを指定
	 * 第2引数はブートローダが使うための領域なら普通はEfiLoaderDataを指定する
	 * 第3引数はページ単位で指定するため、バイト単位の値をページ単位の値に変換する
	 * 第4引数はkernel_base_addrのポインタを渡す(AllocateAddress以外の場合の先頭アドレスを知るため)
	 */
	EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
	gBS->AllocatePages(
		AllocateAddress, EfiLoaderData,
		(kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);
	// メモリ領域が確保できたら、kernel_file->Read()を使ってファイル全体を読み込む
	kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
	Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);

	/**
	 * カーネルが起動する前にUEFI BIOSのブートサービスを停止する
	 * gBS->ExitBootServices()を呼び出して、ブートサービスを停止させる
	 * 成功したら、ブートサービスの機能(Print()やファイルやメモリ関連の機能など)を使えなくなる
	 * マップキーが最新でなければ、実行に失敗する
	 * 失敗したら、サイドメモリマップを取得し、そのマップキーを使って再実行する
	 * 2回目も失敗したら停止する
	 */
	EFI_STATUS status;
	status = gBS->ExitBootServices(image_handle, memmap.map_key);
	if (EFI_ERROR(status)) {
		status = GetMemoryMap(&memmap);
		if (EFI_ERROR(status)) {
			Print(L"failed to get memory map: %r\n", status);
			while (1);
		}
		status = gBS->ExitBootServices(image_handle, memmap.map_key);
		if (EFI_ERROR(status)) {
			Print(L"Could not exit boot service: %r\n", status);
			while (1);
		}
	}

	/**
	 * カーネルを起動する
	 * メモリ上でエントリポイントが置いてある場所を計算してエントリポイントを呼び出す
	 * 64ビット用ELFのエントリポイントアドレスは、オフセットから24バイトの位置から8バイト整数として書かれる
	 */
	UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);
	
	// エントリポイントの場所であるentry_addrの値を関数ポインタにキャストし呼び出す
	typedef void EntryPointType(void);
	EntryPointType* entry_point = (EntryPointType*)entry_addr;
	entry_point();

	// 完了メッセージを表示
	Print(L"All done\n");

	// 無限ループでプログラムを停止（デバッグ・確認用）
	while (1);

	return EFI_SUCCESS;
}