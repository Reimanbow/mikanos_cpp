/**
 * @file memory_manager.cpp
 */
#include "memory_manager.hpp"

BitmapMemoryManager::BitmapMemoryManager()
	: alloc_map_{}, range_begin_{FrameID{0}}, range_end_{FrameID{kFrameCount}} {
}

WithError<FrameID> BitmapMemoryManager::Allocate(size_t num_frames) {
	size_t start_frame_id = range_begin_.ID();
	while (true) {
		// start_frame_idから連続でnum_frames個の空きフレームがあるか確認
		size_t i = 0;
		for (; i < num_frames; ++i) {
			// メモリ範囲の終端を超えたらメモリ不足
			if (start_frame_id + i >= range_end_.ID()) {
				return {kNullFrame, MAKE_ERROR(Error::kNoEnoughMemory)};
			}
			if (GetBit(FrameID{start_frame_id + i})) {
				// start_frame_id + iのフレームは割り当て済み
				// この位置からは連続確保できないので次の位置から再探索
				break;
			}
		}
		if (i == num_frames) {
			// num_frames分の連続した空きフレームが見つかった
			MarkAllocated(FrameID{start_frame_id}, num_frames);
			return {
				FrameID{start_frame_id},
				MAKE_ERROR(Error::kSuccess),
			};
		}
		// 割り当て済みフレームの次の位置から再探索
		start_frame_id += i + 1;
	}
}

Error BitmapMemoryManager::Free(FrameID start_frame, size_t num_frames) {
	for (size_t i = 0; i < num_frames; ++i) {
		SetBit(FrameID{start_frame.ID() + i}, false);
	}
	return MAKE_ERROR(Error::kSuccess);
}

void BitmapMemoryManager::MarkAllocated(FrameID start_frame, size_t num_frames) {
	for (size_t i = 0; i < num_frames; ++i) {
		SetBit(FrameID{start_frame.ID() + i}, true);
	}
}

void BitmapMemoryManager::SetMemoryRange(FrameID range_begin, FrameID range_end) {
	range_begin_ = range_begin;
	range_end_ = range_end;
}

bool BitmapMemoryManager::GetBit(FrameID frame) const {
	// フレームIDからビットマップ配列のインデックスを計算
	auto line_index = frame.ID() / kBitsPerMapLine;  // 配列の何番目の要素か
	auto bit_index = frame.ID() % kBitsPerMapLine;   // その要素内の何ビット目か

	// 該当ビットが1かどうかを判定
	return (alloc_map_[line_index] & (static_cast<MapLineType>(1) << bit_index)) != 0;
}

void BitmapMemoryManager::SetBit(FrameID frame, bool allocated) {
	// フレームIDからビットマップ配列のインデックスを計算
	auto line_index = frame.ID() / kBitsPerMapLine;  // 配列の何番目の要素か
	auto bit_index = frame.ID() % kBitsPerMapLine;   // その要素内の何ビット目か

	if (allocated) {
		// 該当ビットを1にする (割り当て済みとしてマーク)
		alloc_map_[line_index] |= (static_cast<MapLineType>(1) << bit_index);
	} else {
		// 該当ビットを0にする (空きとしてマーク)
		alloc_map_[line_index] &= ~(static_cast<MapLineType>(1) << bit_index);
	}
}