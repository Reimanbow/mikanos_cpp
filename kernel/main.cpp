/**
 * @file main.cpp
 *
 * カーネル本体
 * MikanOSのカーネル（OS本体）のエントリポイント
 */

#include <cstdint>
#include <cstddef>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"

/**
 * @brief 配置new
 * 
 * 一般のnew演算子は、OSがメモリを管理できるようになってはじめてできること
 * 引数に指定したメモリ領域上にインスタンスを生成する
 */
void* operator new(size_t size, void* buf) {
	return buf;
}

void operator delete(void* obj) noexcept {
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

/**
 * @brief カーネルのエントリポイント
 *
 * ブートローダー（MikanLoader）から制御が移った後、最初に実行される関数
 *
 * @param frame_buffer_config フレームバッファの情報を格納した構造体の参照
 */
extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config) {
	switch (frame_buffer_config.pixel_format) {
		case kPixelRGBResv8BitPerColor:
			pixel_writer = new(pixel_writer_buf)
				RGBResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
		case kPixelBGRResv8BitPerColor:
			pixel_writer = new(pixel_writer_buf)
				BGRResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
	}

	// 画面全体を白で塗りつぶす
	for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
		for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
			pixel_writer->Write(x, y, {255, 255, 255});
		}
	}

	// 200 x 100の緑の四角形を描画
	for (int x = 0; x < 200; ++x) {
		for (int y = 0; y < 100; ++y) {
			pixel_writer->Write(100 + x, 100 + y, {0, 255, 0});
		}
	}

	int i = 0;
	for (char c = '!'; c <= '~'; ++c, ++i) {
		WriteAscii(*pixel_writer, 8 * i, 50, c, {0, 0, 0});
	}

	// 無限ループでCPUを停止
	// hlt命令でCPUを省電力モードにする（割り込みが来るまで待機）
	while (1) __asm__("hlt");
}