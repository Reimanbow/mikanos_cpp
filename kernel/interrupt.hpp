/**
 * @file interrupt.hpp
 *
 * 割り込み用のプログラムを集めたファイル．
 */

#pragma once

#include <array>
#include <cstdint>

enum class DescriptorType {
	kUpper8Bytes	= 0,
	kLDT			= 2,
	kTSSAvailable	= 9,
	kTSSBusy		= 11,
	kCallGate		= 12,
	kInterruptGate	= 14,
	kTrapGate		= 15,
};

// IDTの各要素、割り込み記述子の属性を表す共用体
// packed属性は、構造体の各フィールドを詰めて配置するためのコンパイラ拡張
union InterruptDescriptorAttribute {
	uint16_t data;
	struct {
		uint16_t interrupt_stack_table : 3;
		uint16_t : 5;
		DescriptorType type : 4;					// 識別子の種類
		uint16_t : 1;
		uint16_t descriptor_privilege_level : 2;	// 割り込みハンドラの実行権限
		uint16_t present : 1;
	} __attribute__((packed)) bits;
} __attribute__((packed));

// 割り込み記述子そのものを表す構造体
// packed属性は、構造体の各フィールドを詰めて配置するためのコンパイラ拡張
struct InterruptDescriptor {
	uint16_t offset_low;				// 割り込みハンドラのアドレスを設定するフィールド
	uint16_t segment_selector;			// 割り込みハンドラを実行する際のコードセグメント
	InterruptDescriptorAttribute attr;	// 割り込み記述子の属性
	uint16_t offset_middle;				// 割り込みハンドラのアドレスを設定するフィールド
	uint32_t offset_high;				// 割り込みハンドラのアドレスを設定するフィールド
	uint32_t reserved;
} __attribute__((packed));

extern std::array<InterruptDescriptor, 256> idt;

constexpr InterruptDescriptorAttribute MakeIDTAttr(
		DescriptorType type,
		uint8_t descriptor_privilege_level,
		bool present = true,
		uint8_t interrupt_stack_table = 0) {
	InterruptDescriptorAttribute attr{};
	attr.bits.interrupt_stack_table = interrupt_stack_table;
	attr.bits.type = type;
	attr.bits.descriptor_privilege_level = descriptor_privilege_level;
	attr.bits.present = present;
	return attr;
}

void SetIDTEntry(InterruptDescriptor& desc,
				 InterruptDescriptorAttribute attr,
				 uint64_t offset,
				 uint16_t segment_selector);

class InterruptVector {
public:
	enum Number {
		kXHCI = 0x40,
	};
};

struct InterruptFrame {
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

void NotifyEndOfInterrupt();