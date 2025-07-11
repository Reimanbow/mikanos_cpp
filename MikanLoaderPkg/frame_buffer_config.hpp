#pragma once

#include <stdint.h>

// フレームバッファにどういう色の順番で、それぞれ何ビットで並べるのか
enum PixelFormat{
	kPixelRGBResv8BitPerColor,
	kPixelBGRResv8BitPerColor,
};

struct FrameBufferConfig {
	// フレームバッファ領域へのポインタ
	uint8_t* frame_buffer;
	// フレームバッファの余白を含めた横方向のピクセル数
	uint32_t pixels_per_scan_line;
	// 水平方向の解像度
	uint32_t horizontal_resolution;
	// 垂直方向の解像度
	uint32_t vertical_resolution;
	enum PixelFormat pixel_format;
};