#pragma once

#include <stdint.h>

// UEFIの規格におけるピクセルのデータ形式
enum PixelFormat {
	kPixelRGBResv8BitPerColor,	// 1ピクセル32ビット: [R 8bit][G 8bit][B 8bit][予約 8bit]
	kPixelBGRResv8BitPerColor,	// 1ピクセル32ビット: [B 8bit][G 8bit][R 8bit][予約 8bit]
};

// ピクセル描画に必要な情報をまとめるための構造体
struct FrameBufferConfig {
	uint8_t* frame_buffer;			// フレームバッファ（画面描画用メモリ領域）の先頭アドレス
	uint32_t pixels_per_scan_line;	// 1行あたりのピクセル数（余白を含む実際のメモリ上の幅）
	uint32_t horizontal_resolution;	// 画面の横方向の解像度（実際に表示される幅）[ピクセル]
	uint32_t vertical_resolution;	// 画面の縦方向の解像度（実際に表示される高さ）[ピクセル]
	enum PixelFormat pixel_format;	// ピクセルのデータ形式（RGB配置かBGR配置か）
};