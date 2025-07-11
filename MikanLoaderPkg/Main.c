#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>

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

	Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
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

EFI_STATUS OpenGOP(EFI_HANDLE image_handle,
				   EFI_GRAPHICS_OUTPUT_PROTOCOL** gop) {
	UINTN num_gop_handles = 0;
	EFI_HANDLE* gop_handles = NULL;
	gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiGraphicsOutputProtocolGuid,
		NULL,
		&num_gop_handles,
		&gop_handles);
	
	gBS->OpenProtocol(
		gop_handles[0],
		&gEfiGraphicsOutputProtocolGuid,
		(VOID**)gop,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	
	FreePool(gop_handles);

	return EFI_SUCCESS;
}

const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt) {
	switch (fmt) {
		case PixelRedGreenBlueReserved8BitPerColor:
			return L"PixelRedGreenBlueReserved8BitPerColor";
		case PixelBlueGreenRedReserved8BitPerColor:
			return L"PixelBlueGreenRedReserved8BitPerColor";
		case PixelBitMask:
			return L"PixelBitMask";
		case PixelBltOnly:
			return L"PixelBltOnly";
		case PixelFormatMax:
			return L"PixelFormatMax";
		default:
			return L"InvalidPixelFormat";
	}
}

/** @brief プログラムの実行を無限ループで止める */
void Halt(void) {
	while (1) __asm__("hlt");
}

/** @brief UEFIアプリケーションのエントリポイント
 *
 * このローダーのメイン関数。メモリマップを取得し、ファイルに保存する。
 */
EFI_STATUS EFIAPI UefiMain(
		EFI_HANDLE image_handle,
		EFI_SYSTEM_TABLE *system_table) {
	EFI_STATUS status;
	Print(L"Hello, Mikan World!\n");

	// メモリマップを格納するためのバッファを準備
	CHAR8 memmap_buf[4096 * 4];
	struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
	// メモリマップを取得
	status = GetMemoryMap(&memmap);
	if (EFI_ERROR(status)) {
		Print(L"failed to get memory map: %r\n", status);
		Halt();
	}

	// ルートディレクトリを開く
	EFI_FILE_PROTOCOL* root_dir;
	status = OpenRootDir(image_handle, &root_dir);
	if (EFI_ERROR(status)) {
		Print(L"failed to open root directory: %r\n", status);
		Halt();
	}

	// メモリマップを保存するためのファイルを開く (または作成する)
	EFI_FILE_PROTOCOL* memmap_file;
	status = root_dir->Open(
		root_dir, &memmap_file, L"\\memmap",
		EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
	if (EFI_ERROR(status)) {
		Print(L"failed to open file '\\memmap': %r\n", status);
		Print(L"Ignored.\n");
	} else {
		status = SaveMemoryMap(&memmap, memmap_file);
		if (EFI_ERROR(status)) {
			Print(L"failed to save memory map: %r\n", status);
			Halt();
		}
		status = memmap_file->Close(memmap_file);
		if (EFI_ERROR(status)) {
			Print(L"failed to close memory map: %r\n", status);
			Halt();
		}
	}

	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
	// OpenGOP()を使ってGOPを取得する
	status = OpenGOP(image_handle, &gop);
	if (EFI_ERROR(status)) {
		Print(L"failed to open GOP: %r\n", status);
		Halt();
	}

	Print(L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
		gop->Mode->Info->HorizontalResolution,
		gop->Mode->Info->VerticalResolution,
		GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
		gop->Mode->Info->PixelsPerScanLine);
	Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
		gop->Mode->FrameBufferBase,
		gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
		gop->Mode->FrameBufferSize);
	
	// フレームバッファの先頭アドレス（gop->Mode->Info->FrameBufferBase）と全体サイズ（gop->Mode->FrameBufferSize）を使って画面を塗りつぶす
	// すべてのバイトに255を書き白に塗りつぶす
	UINT8* frame_buffer = (UINT8*)gop->Mode->FrameBufferBase;
	for (UINTN i = 0; i < gop->Mode->FrameBufferSize; ++i) {
		frame_buffer[i] = 255;
	}

	// トップディレクトリにあるkernel.elfという名前のファイルを読み込み専用で開く
	EFI_FILE_PROTOCOL* kernel_file;
	status = root_dir->Open(
		root_dir, &kernel_file, L"\\kernel.elf",
		EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status)) {
		Print(L"failed to open file '\\kernel.elf': %r\n", status);
		Halt();
	}
	
	// 開いたファイル全体を読み込むためのメモリを用意する
	// kernel_file->GetInfo()を使ってファイル情報を取得する
	// EFI_FILE_INFO型を格納できる大きさの領域を確保する:sizeof(CHAR16)*12バイト
	// EFI_FILE_INFO型の最後の要素であるFileNameの大きさは可変のため, \kernel.elfという12文字を格納できるようにsizeof(CHAR16)*12バイトだけ大きい領域を確保する
	UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
	UINT8 file_info_buffer[file_info_size];
	status = kernel_file->GetInfo(
		kernel_file, &gEfiFileInfoGuid,
		&file_info_size, file_info_buffer);
	if (EFI_ERROR(status)) {
		Print(L"failed to get file information: %r\n", status);
		Halt();
	}
	
	// カーネルファイルの大きさが分かったら、ファイルを格納できる十分な大きさのメモリを確保する
	EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
	UINTN kernel_file_size = file_info->FileSize;

	// AllocatePagesを使ってファイルを格納できる十分な大きさのメモリを確保する
	// 第1引数にメモリの確保の仕方、第2引数に確保するメモリ領域の種別、第3引数に大きさ、第4引数に確保したメモリ領域のアドレス
	// AllocateAddress: 指定したアドレス（0x100000）に確保する(ld.lldのオプションで指定済み)
	// 第3引数の大きさはページ単位なので、バイト単位からページ単位に変換する
	EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
	status = gBS->AllocatePages(
		AllocateAddress, EfiLoaderData,
		(kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);
	if (EFI_ERROR(status)) {
		Print(L"failed to allocate pages: %r\n", status);
		Halt();
	}
	// メモリ領域が確保できたらkernel_file→Read()を使ってファイル全体を読み込む
	status = kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
	if (EFI_ERROR(status)) {
		Print(L"error: %r\n", status);
		Halt();
	}
	Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);

	// UEFI BIOSのブートサービスを停止する
	// ExitBootServices()は、その呼び出し時点で最新のメモリマップのマップキーを要求する
	// 指定されたマップキーが最近のメモリマップに紐づくマップキーでない場合、実行に失敗する
	status = gBS->ExitBootServices(image_handle, memmap.map_key);
	// 失敗したら、再度メモリマップを取得し、そのマップキーを使って再実行する
	if (EFI_ERROR(status)) {
		status = GetMemoryMap(&memmap);
		if (EFI_ERROR(status)) {
			Print(L"Failed to get memory map: %r\n", status);
			Halt();
		}
		// 2回目も失敗したら重大なエラーなので、永久ループで停止する
		status = gBS->ExitBootServices(image_handle, memmap.map_key);
		if (EFI_ERROR(status)) {
			Print(L"Could not exit boot service: %r\n", status);
			Halt();
		}
	}

	// カーネルを起動する
	// メモリ上でエントリポイント(KernelMain())が置いてある場所を計算してエントリポイントを呼び出す
	// readelf -hより、エントリポイントアドレスの値が0x101120である
	// 64ビット用のELFのエントリポイントアドレスは、オフセット24バイトの位置から8バイト整数として書かれる
	UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);

	// エントリポイントの場所であるentry_addrを関数ポインタにキャストして呼び出す
	// 引数と戻り値がどちらもvoid型であるような関数を表すEntryPointTypeという型を作成することで、C言語の関数として呼び出せる
	typedef void EntryPointType(UINT64, UINT64);
	EntryPointType* entry_point = (EntryPointType*)entry_addr;
	entry_point(gop->Mode->FrameBufferBase, gop->Mode->FrameBufferSize);

	Print(L"All done\n");

	// OSに処理を渡すまで無限ループで停止
	while (1);
	return EFI_SUCCESS;
}
