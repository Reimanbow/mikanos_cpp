#pragma once

#include <stdint.h>

// UEFIの規格におけるピクセルのデータ形式
enum PixelFormat {
	kPixelRGBResv8BitPerColor,	// 1ピクセル32ビット: [R 8bit][G 8bit][B 8bit][予約 8bit]
	kPixelBGRResv8BitPerColor,	// 1ピクセル32ビット: [B 8bit][G 8bit][R 8bit][予約 8bit]
};

// ピクセル描画に必要な情報をまとめるための構造体
struct FrameBufferConfig {
	uint8_t* frame_buffer;			// フレームバッファの先頭アドレス
	uint32_t pixels_per_scan_line;	// 余白を含めた
	uint32_t horizontal_resolution;	//
	uint32_t vertical_resolution;	//
	enum PixelFormat pixel_format;	//
};