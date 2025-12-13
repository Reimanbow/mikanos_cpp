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
	Error Copy(Vector2D<int> pos, const FrameBuffer& src);

	FrameBufferWriter& Writer() { return *writer_; }

private:
	// 描画領域に関する構成情報
	FrameBufferConfig config_{};
	// ピクセルの配列で、描画領域の本体
	std::vector<uint8_t> buffer_{};
	std::unique_ptr<FrameBufferWriter> writer_{};
};

/**
 * @brief 1ピクセルのビット数を返す
 */
int BitsPerPixel(PixelFormat format);