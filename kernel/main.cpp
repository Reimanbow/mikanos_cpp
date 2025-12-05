/**
 * @file main.cpp
 *
 * カーネル本体
 * MikanOSのカーネル（OS本体）のエントリポイント
 */

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"

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

char console_buf[sizeof(Console)];
Console* console;

int printk(const char* format, ...) {
	va_list ap;
	int result;
	char s[1024];

	va_start(ap, format);
	result = vsprintf(s, format, ap);
	va_end(ap);

	console->PutString(s);
	return result;
}

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

	console = new(console_buf) Console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};

	for (int i = 0; i < 27; ++i) {
		printk("printk: %d\n", i);
	}

	// 無限ループでCPUを停止
	// hlt命令でCPUを省電力モードにする（割り込みが来るまで待機）
	while (1) __asm__("hlt");
}