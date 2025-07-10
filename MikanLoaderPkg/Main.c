#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>

/** @brief メモリマップの情報を格納する構造体
 *
 * UEFIのGetMemoryMap()が返す情報をこの構造体でやり取りする。
 */
struct MemoryMap {
	UINTN buffer_size;        //!< bufferのバイト数
	VOID* buffer;             //!< メモリマップ記述子を格納するバッファ
	UINTN map_size;           //!< 実際のメモリマップのバイト数
	UINTN map_key;            //!< ExitBootServices()に渡すためのキー
	UINTN descriptor_size;    //!< メモリマップ記述子1個のバイト数
	UINT32 descriptor_version; //!< メモリマップ記述子のバージョン
};

/** @brief システムのメモリマップを取得する
 *
 * UEFI Boot ServiceのGetMemoryMap()を呼び出し、メモリマップを取得して
 * 引数の map に格納する。
 *
 * @param map メモリマップの情報を格納する構造体へのポインタ
 * @return EFI_STATUS */
EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
	if (map->buffer == NULL) {
		return EFI_BUFFER_TOO_SMALL;
	}

	map->map_size = map->buffer_size;
	return gBS->GetMemoryMap(
		&map->map_size,
		(EFI_MEMORY_DESCRIPTOR*)map->buffer,
		&map->map_key,
		&map->descriptor_size,
		&map->descriptor_version);
}

/** @brief EFI_MEMORY_TYPEに対応する文字列を返す
 *
 * メモリマップを人間が読める形式でファイルに保存するために使用する。
 *
 * @param type メモリタイプを示すenum値
 * @return メモリタイプを示すワイド文字列 */
const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
	switch (type) {
		case EfiReservedMemoryType: return L"EfiReservedMemoryType";
		case EfiLoaderCode: return L"EfiLoaderCode";
		case EfiLoaderData: return L"EfiLoaderData";
		case EfiBootServicesCode: return L"EfiBootServicesCode";
		case EfiBootServicesData: return L"EfiBootServicesData";
		case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
		case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
		case EfiConventionalMemory: return L"EfiConventionalMemory";
		case EfiUnusableMemory: return L"EfiUnusableMemory";
		case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
		case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
		case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
		case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
		case EfiPalCode: return L"EfiPalCode";
		case EfiPersistentMemory: return L"EfiPersistentMemory";
		case EfiMaxMemoryType: return L"EfiMaxMemoryType";
		default: return L"InvalidMemoryType";
	}
}

/** @brief メモリマップを指定されたファイルに保存する
 *
 * メモリマップの各エントリをCSV形式でファイルに書き出す。
 *
 * @param map 保存するメモリマップの情報
 * @param file 書き込み先のファイル
 * @return EFI_STATUS */
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file) {
	CHAR8 buf[256];
	UINTN len;

	// CSVファイルの1行目となるヘッダーを定義し、ファイルに書き込む
	CHAR8* header =
		"Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
	len = AsciiStrLen(header);
	file->Write(file, &len, header);

	Print(L"map->buffer = %08lx, map->map_size = %8lx\n",
			map->buffer, map->map_size);

	EFI_PHYSICAL_ADDRESS iter;
	int i;
	// メモリマップのバッファを先頭から末尾までループ処理する
	// iterはディスクリプタのサイズ(descriptor_size)ずつ進む
	for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
		 iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
		 iter += map->descriptor_size, i++) {
		// 現在位置のメモリ領域をEFI_MEMORY_DESCRIPTOR構造体として解釈する
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;

		// 1つのディスクリプタの情報をCSV形式の1行の文字列にフォーマットする
		len = AsciiSPrint(
			buf, sizeof(buf),
			"%u, %x, %-ls, %08lx, %lx, %lx\n",
			i, desc->Type, GetMemoryTypeUnicode(desc->Type),
			desc->PhysicalStart, desc->NumberOfPages,
			desc->Attribute & 0xffffflu);
		// フォーマットした文字列をファイルに書き込む
		file->Write(file, &len, buf);
	}

	return EFI_SUCCESS;
}

/** @brief ルートディレクトリを開く
 *
 * このUEFIアプリケーションがロードされたデバイスのファイルシステムの
 * ルートディレクトリを開き、そのプロトコルを返す。
 *
 * @param image_handle このUEFIアプリケーションのイメージハンドル
 * @param root ルートディレクトリのファイルプロトコルを格納するポインタ
 * @return EFI_STATUS */
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
	EFI_LOADED_IMAGE_PROTOCOL* loaded_image; // このイメージ自身に関するプロトコル
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

	// 手順1: Loaded Image Protocol を取得する
	// このプロトコルには、このUEFIアプリケーションがどのデバイスからロードされたか
	// という情報 (DeviceHandle) が含まれている。
	gBS->OpenProtocol(
		image_handle,
		&gEfiLoadedImageProtocolGuid,
		(VOID**)&loaded_image,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

	// 手順2: Simple File System Protocol を取得する
	// 手順1で取得した DeviceHandle を使い、そのデバイスのファイルシステムを
	// 操作するためのプロトコルを取得する。
	gBS->OpenProtocol(
		loaded_image->DeviceHandle,
		&gEfiSimpleFileSystemProtocolGuid,
		(VOID**)&fs,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

	// 手順3: ファイルシステムのルートディレクトリを開く
	// これにより、ファイルやディレクトリを操作するためのプロトコル (EFI_FILE_PROTOCOL) が得られる。
	// 結果は引数の root ポインタに格納される。
	fs->OpenVolume(fs, root);

	return EFI_SUCCESS;
}

/** @brief UEFIアプリケーションのエントリポイント
 *
 * このローダーのメイン関数。メモリマップを取得し、ファイルに保存する。
 */
EFI_STATUS EFIAPI UefiMain(
		EFI_HANDLE image_handle,
		EFI_SYSTEM_TABLE *system_table) {
	Print(L"Hello, Mikan World!\n");

	// メモリマップを格納するためのバッファを準備
	CHAR8 memmap_buf[4096 * 4];
	struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
	// メモリマップを取得
	GetMemoryMap(&memmap);

	// ルートディレクトリを開く
	EFI_FILE_PROTOCOL* root_dir;
	OpenRootDir(image_handle, &root_dir);

	// メモリマップを保存するためのファイルを開く (または作成する)
	EFI_FILE_PROTOCOL* memmap_file;
	root_dir->Open(
		root_dir, &memmap_file, L"\\memmap",
		EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

	// ファイルにメモリマップを保存
	SaveMemoryMap(&memmap, memmap_file);
	memmap_file->Close(memmap_file);

	Print(L"All done\n");

	// OSに処理を渡すまで無限ループで停止
	while (1);
	return EFI_SUCCESS;
}
