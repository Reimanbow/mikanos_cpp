/**
 * @file memory_map.hpp
 */
#pragma once

#include <stdint.h>

/**
 * @brief メモリマップ情報を格納する構造体
 *
 * UEFIファームウェアが管理しているメモリマップ（どのメモリ領域が何に使われているか）を取得・保持するためのデータ構造
 * OSを起動する際には、このメモリマップ情報をOSに渡す必要がある
 */
struct MemoryMap {
	unsigned long long buffer_size;			// メモリマップを格納するバッファのサイズ（バイト）
	void* buffer;							// メモリマップデータを格納するバッファへのポインタ
	unsigned long long map_size;			// 実際のメモリマップのサイズ（GetMemoryMap実行後に設定される）
	unsigned long long map_key;				// メモリマップの状態を識別するキー（ExitBootServices時に必要）
	unsigned long long descriptor_size;		// 各メモリディスクリプタ（1エントリ）のバイト数
	uint32_t descriptor_version;			// メモリディスクリプタ構造体のバージョン
};

/**
 * @brief メモリディスクリプタ（メモリマップの1エントリ）
 *
 * メモリマップの各エントリを表す構造体
 * 物理メモリの連続した領域（1つのメモリ範囲）の情報を保持する
 */
struct MemoryDescriptor {
	uint32_t type;					// メモリ領域の種類（MemoryType enumの値）
	uintptr_t physical_start;		// 物理メモリの開始アドレス
	uintptr_t virtual_start;		// 仮想メモリの開始アドレス（OSが設定する）
	uint64_t number_of_pages;		// ページ数（1ページ = 4096バイト）
	uint64_t attribute;				// メモリ属性（読み書き可能、実行可能など）
};

#ifdef __cplusplus
/**
 * @brief メモリ領域の種類を表すenum
 *
 * UEFIが定義するメモリタイプ
 * OSはこれらを見て、どのメモリ領域が使用可能かを判断する
 */
enum class MemoryType {
	kEfiReservedMemoryType,			// 予約済み（使用不可）
	kEfiLoaderCode,					// ブートローダーのコード領域
	kEfiLoaderData,					// ブートローダーのデータ領域
	kEfiBootServicesCode,			// UEFIブートサービスのコード（OS起動後は解放可能）
	kEfiBootServicesData,			// UEFIブートサービスのデータ（OS起動後は解放可能）
	kEfiRuntimeServicesCode,		// UEFIランタイムサービスのコード（OS起動後も使用）
	kEfiRuntimeServicesData,		// UEFIランタイムサービスのデータ（OS起動後も使用）
	kEfiConventionalMemory,			// 通常のメモリ（OSが自由に使える）
	kEfiUnusableMemory,				// 使用不可能なメモリ
	kEfiACPIReclaimMemory,			// ACPI情報格納領域（読み取り後は解放可能）
	kEfiACPIMemoryNVS,				// ACPI NVS（不揮発性ストレージ、保持が必要）
	kEfiMemoryMappedIO,				// メモリマップドI/O領域
	kEfiMemoryMappedIOPortSpace,	// メモリマップドI/Oポート空間
	kEfiPalCode,					// プロセッサ固有のファームウェアコード
	kEfiPersistentMemory,			// 永続メモリ
	kEfiMaxMemoryType				// 最大値（列挙子の数）
};

/**
 * @brief uint32_tとMemoryTypeの比較演算子
 *
 * MemoryDescriptor::type（uint32_t）とMemoryType enumを直接比較できるようにする
 */
inline bool operator==(uint32_t lhs, MemoryType rhs) {
	return lhs == static_cast<uint32_t>(rhs);
}

/**
 * @brief MemoryTypeとuint32_tの比較演算子
 *
 * 上記の逆順バージョン
 */
inline bool operator==(MemoryType lhs, uint32_t rhs) {
	return rhs == lhs;
}
#endif