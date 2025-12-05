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

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

/**
 * マウスカーソルの形の定義
 * '@'がマウスカーソルの縁、'.'がマウスカーソルの内側
 * 配列の横方向は文字列末尾のヌル文字を格納するため
 */
const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
  "@              ",
  "@@             ",
  "@.@            ",
  "@..@           ",
  "@...@          ",
  "@....@         ",
  "@.....@        ",
  "@......@       ",
  "@.......@      ",
  "@........@     ",
  "@.........@    ",
  "@..........@   ",
  "@...........@  ",
  "@............@ ",
  "@......@@@@@@@@",
  "@......@       ",
  "@....@@.@      ",
  "@...@ @.@      ",
  "@..@   @.@     ",
  "@.@    @.@     ",
  "@@      @.@    ",
  "@       @.@    ",
  "         @.@   ",
  "         @@@   ",
};

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

	const int kFrameWidth = frame_buffer_config.horizontal_resolution;
	const int kFrameHeight = frame_buffer_config.vertical_resolution;

	FillRectangle(*pixel_writer,
				  {0, 0},
				  {kFrameWidth, kFrameHeight},
				  kDesktopBGColor);
	FillRectangle(*pixel_writer,
				  {0, kFrameHeight - 50},
				  {kFrameWidth, 50},
				  {1, 8, 17});
	FillRectangle(*pixel_writer,
				  {0, kFrameHeight - 50},
				  {kFrameWidth / 5, 50},
				  {80, 80, 80});
	DrawRectangle(*pixel_writer,
				  {10, kFrameHeight - 40},
				  {30, 30},
				  {160, 160, 160});

	console = new(console_buf) Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};
	printk("Welcome to MikanOS!\n");

	for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
		for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
			if (mouse_cursor_shape[dy][dx] == '@') {
				pixel_writer->Write(200 + dx, 100 + dy, {0, 0, 0});
			} else if (mouse_cursor_shape[dy][dx] == '.') {
				pixel_writer->Write(200 + dx, 100 + dy, {255, 255, 255});
			}
		}
	}

	// 無限ループでCPUを停止
	// hlt命令でCPUを省電力モードにする（割り込みが来るまで待機）
	while (1) __asm__("hlt");
}