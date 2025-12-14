/**
 * @file frame_buffer.hpp
 */
#pragma once

#include <vector>
#include <memory>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "error.hpp"

class FrameBuffer {
public:
	/**
	 * @brief 指定された設定でバッファを確保する
	 */
	Error Initialize(const FrameBufferConfig& config);

	/**
	 * @brief 指定されたバッファを自身のバッファへコピーする
	 */
	Error Copy(Vector2D<int> dst_pos, const FrameBuffer& src, const Rectangle<int>& src_area);

	/**
	 * @brief 指定された範囲を移動させる
	 */
	void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);

	FrameBufferWriter& Writer() { return *writer_; }

private:
	// 描画領域に関する構成情報
	FrameBufferConfig config_{};
	// ピクセルの配列で、描画領域の本体
	std::vector<uint8_t> buffer_{};
	std::unique_ptr<FrameBufferWriter> writer_{};
};
