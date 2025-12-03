/**
 * @file main.cpp
 *
 * カーネル本体
 * MikanOSのカーネル（OS本体）のエントリポイント
 */

#include <cstdint>
#include <cstddef>

#include "frame_buffer_config.hpp"

// ピクセルの色を設定する構造体
struct PixelColor {
	uint8_t r, g, b;	// [R 8bit][G 8bit][B 8bit]
};

/**
 * @brief 1つの点を描画する
 * @retval 0	成功
 * @retval 非0	失敗
 */
int WritePixel(const FrameBufferConfig& config,
				int x, int y, const PixelColor& c) {
	// ピクセルの座標: 余白を含めた横方向のピクセル数 * y + x
	const int pixel_position = config.pixels_per_scan_line * y + x;
	if (config.pixel_format == kPixelRGBResv8BitPerColor) {
		// 1ピクセルの大きさは4バイト
		uint8_t* p = &config.frame_buffer[4 * pixel_position];
		p[0] = c.r;
		p[1] = c.g;
		p[2] = c.b;
	} else if (config.pixel_format == kPixelBGRResv8BitPerColor) {
		uint8_t* p = &config.frame_buffer[4 * pixel_position];
		p[0] = c.b;
		p[1] = c.g;
		p[2] = c.r;
	} else {
		return -1;
	}
	return 0;
}

/**
 * @brief カーネルのエントリポイント
 *
 * ブートローダー（MikanLoader）から制御が移った後、最初に実行される関数
 *
 * @param frame_buffer_config フレームバッファの情報を格納した構造体の参照
 */
extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config) {
	// 画面全体を白で塗りつぶす
	for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
		for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
			WritePixel(frame_buffer_config, x, y, {255, 255, 255});
		}
	}

	// 200 x 100の緑の四角形を描画
	for (int x = 0; x < 200; ++x) {
		for (int y = 0; y < 100; ++y) {
			WritePixel(frame_buffer_config, 100 + x, 100 + y, {0, 255, 0});
		}
	}

	// 無限ループでCPUを停止
	// hlt命令でCPUを省電力モードにする（割り込みが来るまで待機）
	while (1) __asm__("hlt");
}