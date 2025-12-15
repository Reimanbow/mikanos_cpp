/**
 * @file interrupt.hpp
 */
#pragma once

#include <array>
#include <cstdint>
#include <deque>

#include "x86_descriptor.hpp"
#include "message.hpp"

// 割り込み記述子の属性を表す共用体
union InterruptDescriptorAttribute {
	uint16_t data;
	struct {
		uint16_t interrupt_stack_table : 3;			// 常に0
		uint16_t : 5;
		DescriptorType type : 4;					// 記述子の種別
		uint16_t : 1;
		uint16_t descriptor_privilege_level : 2;	// 割り込みハンドラの実行権限
		uint16_t present : 1;						// 記述子が有効であることを示すフラグ
	} __attribute__((packed)) bits;
} __attribute__((packed));

// 割り込み記述子を表す構造体
struct InterruptDescriptor {
	uint16_t offset_low;				// 割り込みハンドラのアドレス
	uint16_t segment_selector;			// 割り込みハンドラを実行する際のコードセグメント
	InterruptDescriptorAttribute attr;	// 割り込み記述子の属性
	uint16_t offset_middle;				// 割り込みハンドラのアドレス
	uint32_t offset_high;				// 割り込みハンドラのアドレス
	uint32_t reserved;
} __attribute__((packed));

extern std::array<InterruptDescriptor, 256> idt;

/**
 * @brief 割り込み記述子の属性を作成する
 */
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

/**
 * @brief 割り込み記述子に各種の設定を行う
 * 
 * @param desc				設定する割り込み記述子
 * @param attr				記述子の属性
 * @param offset			割り込みハンドラのアドレス
 * @param segment_selector	割り込みハンドラがおかれたコードセグメントのセレクタ値
 */
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

/**
 * @brief End of Interruptレジスタ(0xfee000b0番地)に0を書き込む
 * 
 * この番地に値を書くことで割り込み処理の終了をCPUにも伝えられるようになっている
 */
void NotifyEndOfInterrupt();

void InitializeInterrupt(std::deque<Message>* msg_queue);