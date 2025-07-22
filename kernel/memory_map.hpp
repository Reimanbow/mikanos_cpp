// Main.cとmain.cppから利用されるので、CとしてもC++としても利用できるようにしておく必要がある

#pragma once

#include <stdint.h>

/** 
 * メモリマップの情報を格納する構造体
 */
struct MemoryMap {
	unsigned long long buffer_size;		//!< bufferのバイト数
	void* buffer;						//!< メモリマップ記述子を格納するバッファ
	unsigned long long map_size;		//!< メモリマップ記述子を格納するバッファ
	unsigned long long map_key;			//!< ExitBootServices()に渡すためのキー
	unsigned long long descriptor_size;	//!< メモリマップ記述子1個のバイト数
	uint32_t descriptor_version;		//!< メモリマップ記述子のバージョン
};

struct MemoryDescriptor {
	uint32_t type;
	uintptr_t physical_start;
	uintptr_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attribute;
};

// 以下はC++特有の機能
#ifdef __cplusplus
enum class MemoryType {
	kEfiReservedMemoryType,
	kEfiLoaderCode,
	kEfiLoaderData,
	kEfiBootServicesCode,
	kEfiBootServicesData,
	kEfiRuntimeServicesCode,
	kEfiRuntimeServicesData,
	kEfiConventionalMemory,
	kEfiUnusableMemory,
	kEfiACPIReclaimMemory,
	kEfiACPIMemoryNVS,
	kEfiMemoryMappedIO,
	kEfiMemoryMappedIOPortSpace,
	kEfiPalCode,
	kEfiPersistentMemory,
	kEfiMaxMemoryType	
};

inline bool operator==(uint32_t lhs, MemoryType rhs) {
	return lhs == static_cast<uint32_t>(rhs);
}

inline bool operator==(MemoryType lhs, uint32_t rhs) {
	return rhs == lhs;
}

#endif