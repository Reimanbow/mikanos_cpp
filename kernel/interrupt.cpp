/**
 * @file interrupt.cpp
 *
 * 割り込み用のプログラムを集めたファイル．
 */

#include "interrupt.hpp"

std::array<InterruptDescriptor, 256> idt;

// descで指定された割り込み記述子に各種の設定を行う
// 記述子の属性、割り込みハンドラのアドレス、割り込みハンドラが置かれたコードセグメントのセレクタ値を設定
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
	// End of Interruptレジスタ（0xfee000b0番地）に0を書き込む
	// この番地に値を書くことで割り込み処理の終了をCPUに伝えられる
	// volatileを付けて、最適化の対照から外す
	volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(0xfee000b0);
	*end_of_interrupt = 0;
}