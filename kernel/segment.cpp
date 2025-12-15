/**
 * @file segment.cpp
 */
#include "segment.hpp"

#include "asmfunc.h"

namespace {
	std::array<SegmentDescriptor, 3> gdt;
}

void SetCodeSegment(SegmentDescriptor& desc,
					DescriptorType type,
					unsigned int descriptor_privilege_level,
					uint32_t base,
					uint32_t limit) {
	desc.data = 0;

	desc.bits.base_low = base & 0xffffu;
	desc.bits.base_middle = (base >> 16) & 0xffu;
	desc.bits.base_high = (base >> 24) & 0xffu;

	desc.bits.limit_low = limit & 0xffffu;
	desc.bits.limit_high = (limit >> 16) & 0xfu;

	desc.bits.type = type;
	desc.bits.system_segment = 1;
	desc.bits.descriptor_privilege_level = descriptor_privilege_level;
	desc.bits.present = 1;
	desc.bits.available = 0;
	desc.bits.long_mode = 1;
	desc.bits.default_operation_size = 0;
	desc.bits.granularity = 1;
}

void SetDataSegment(SegmentDescriptor& desc,
					DescriptorType type,
					unsigned int descriptor_privilege_level,
					uint32_t base,
					uint32_t limit) {
	SetCodeSegment(desc, type, descriptor_privilege_level, base, limit);
	desc.bits.long_mode = 0;
	desc.bits.default_operation_size = 1;
}
				
/**
 * @brief GDTを再構築する
 * 
 * GDTにある3つのディスクリプタに値を設定している
 */
void SetupSegments() {
	// ヌルディスクリプタとして設定。すべて0で埋める
	gdt[0].data = 0;
	// コードセグメントディスクリプタとして設定
	SetCodeSegment(gdt[1], DescriptorType::kExecuteRead, 0, 0, 0xfffff);
	// データセグメントディスクリプタとして設定
	SetDataSegment(gdt[2], DescriptorType::kReadWrite, 0, 0, 0xfffff);
	// 変数gdtを正式なGDTとしてCPUに登録する
	LoadGDT(sizeof(gdt) - 1, reinterpret_cast<uintptr_t>(&gdt[0]));
}

void InitializeSegmentation() {
	SetupSegments();

	SetDSAll(kKernelDS);
	SetCSSS(kKernelCS, kKernelSS);
}