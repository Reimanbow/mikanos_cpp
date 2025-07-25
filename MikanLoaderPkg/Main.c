#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>
#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "elf.hpp"

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

/** @brief カーネルファイル内のすべてのLOADセグメント（p_typeがPT_LOADであるセグメント）を順にめぐり、アドレス範囲を更新していく */
void CalcLoadAddressRange(Elf64_Ehdr* ehdr, UINT64* first, UINT64* last) {
	// プログラムヘッダの配列を指すポインタ
	Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
	*first = MAX_UINT64;
	*last = 0;
	for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
		if (phdr[i].p_type != PT_LOAD) continue;
		// 先頭にあるLOADセグメントのp_vaddrの値になる
		*first = MIN(*first, phdr[i].p_vaddr);
		// 末尾にあるLOADセグメントのp_vaddr+p_memszの値になる
		*last = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz);
	}
}

/** @brief p_type == PT_LOADであるセグメントに対して2つの処理を行う
 * 
 * 1.segm_in_fileが指す一時領域からp_vaddrが指す最終目的地へデータをコピーする
 * 2.セグメントのメモリ上のサイズがファイル上のサイズより大きい場合、残りを0で埋める
 */
void CopyLoadSegments(Elf64_Ehdr* ehdr) {
	Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
	for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
		if (phdr[i].p_type != PT_LOAD) continue;

		UINT64 segm_in_file = (UINT64)ehdr + phdr[i].p_offset;
		CopyMem((VOID*)phdr[i].p_vaddr, (VOID*)segm_in_file, phdr[i].p_filesz);

		UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
		SetMem((VOID*)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
	}
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
	
	// カーネルファイルの読み込み
	EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
	UINTN kernel_file_size = file_info->FileSize;

	VOID* kernel_buffer;
	// カーネルファイルを読み込むための一時領域を確保する. ページ単位ではなくバイト単位でメモリを確保できる. ただし、場所を指定する機能はない
	// 成功すると、kernel_bufferには確保されたメモリ領域の先頭アドレスが格納されている
	status = gBS->AllocatePool(EfiLoaderData, kernel_file_size, &kernel_buffer);
	if (EFI_ERROR(status)) {
		Print(L"failed to allocate pool: %r\n", status);
		Halt();
	}
	// 一時領域の先頭アドレスをkernel_file->Read()に指定することで、カーネルファイルの内容をすべて一次領域へ読み込める
	status = kernel_file->Read(kernel_file, &kernel_file_size, kernel_buffer);
	if (EFI_ERROR(status)) {
		Print(L"error: %r\n", status);
		Halt();
	}

	// コピー先のメモリ領域の確保
	// 最終目的地の番地の範囲を取得し、それに応じて最終目的地のメモリ領域を確保する
	// 最終目的地の範囲は0x10000から始まるアドレスの範囲
	Elf64_Ehdr* kernel_ehdr = (Elf64_Ehdr*)kernel_buffer;
	// 範囲を計算し、kernel_first_addrに開始アドレスを、kernel_last_addrに終了アドレスを設定する
	UINT64 kernel_first_addr, kernel_last_addr;
	CalcLoadAddressRange(kernel_ehdr, &kernel_first_addr, &kernel_last_addr);

	// 必要なメモリの大きさを計算し、メモリを確保する
	UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;
	// AllocatePagesを使ってファイルを格納できる十分な大きさのメモリを確保する
	// 第1引数にメモリの確保の仕方、第2引数に確保するメモリ領域の種別、第3引数に大きさ、第4引数に確保したメモリ領域のアドレス
	// AllocateAddress: 指定したアドレス（0x100000）に確保する(ld.lldのオプションで指定済み)
	// 第3引数の大きさはページ単位なので、バイト単位からページ単位に変換する
	status = gBS->AllocatePages(AllocateAddress, EfiLoaderData,
								num_pages, &kernel_first_addr);
	if (EFI_ERROR(status)) {
		Print(L"failed to allocate pages: %r\n", status);
		Halt();
	}

	// 一時領域から最終目的地へLOADセグメントのコピー
	CopyLoadSegments(kernel_ehdr);
	Print(L"Kernel: 0x%0lx - 0x%0lx\n", kernel_first_addr, kernel_last_addr);

	// 一時領域を解放する
	status = gBS->FreePool(kernel_buffer);
	if (EFI_ERROR(status)) {
		Print(L"failed to free pool: %r\n", status);
		Halt();
	}

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
	UINT64 entry_addr = *(UINT64*)(kernel_first_addr + 24);

	// GOPから取得した情報を、FrameBufferConfigにコピーする
	struct FrameBufferConfig config = {
		(UINT8*)gop->Mode->FrameBufferBase,
		gop->Mode->Info->PixelsPerScanLine,
		gop->Mode->Info->HorizontalResolution,
		gop->Mode->Info->VerticalResolution,
		0
	};
	switch (gop->Mode->Info->PixelFormat) {
		case PixelRedGreenBlueReserved8BitPerColor:
			config.pixel_format = kPixelRGBResv8BitPerColor;
			break;
		case PixelBlueGreenRedReserved8BitPerColor:
			config.pixel_format = kPixelBGRResv8BitPerColor;
			break;
		default:
			Print(L"Unimplemented pixel format: %d\n", gop->Mode->Info->PixelFormat);
			Halt();
	}

	// エントリポイントの場所であるentry_addrを関数ポインタにキャストして呼び出す
	// 関数を表すEntryPointTypeという型を作成することで、C言語の関数として呼び出せる
	// FrameBufferConfigへのポインタをKernelMain()の第1引数に渡す
	// メモリマップ構造体のポインタをKernelMain()に渡す
	typedef void EntryPointType(const struct FrameBufferConfig*,
								const struct MemoryMap*);
	EntryPointType* entry_point = (EntryPointType*)entry_addr;
	entry_point(&config, &memmap);

	Print(L"All done\n");

	// OSに処理を渡すまで無限ループで停止
	while (1);
	return EFI_SUCCESS;
}
