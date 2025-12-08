/**
 * @file interrupt.cpp
 */
#include "interrupt.hpp"

// 割り込み記述子テーブル
std::array<InterruptDescriptor, 256> idt;

void SetIDTEntry(InterruptDescriptor& desc,
				 InterruptDescriptorAttribute attr,
				 uint64_t offset,
				 uint16_t segment_selector) {
	desc.attr = attr;
	desc.offset_low = offset & 0xffffu;
	desc.offset_middle = (offset >> 16) & 0xffffu;
	desc.offset_high = offset >> 32;
	desc.segment_selector = segment_selector;
}

void NotifyEndOfInterrupt() {
	// 0xfee000b0に確実に書き込むためにvolatileを使用する
	volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(0xfee000b0);
	*end_of_interrupt = 0;
}