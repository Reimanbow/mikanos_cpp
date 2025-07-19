/**
 * @file main.cpp
 * 
 * カーネル本体のプログラムを書いたファイル
 */

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"

// これがないとリンク時にエラーとなる
void operator delete(void* obj) noexcept {
}

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

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

// 可変長引数を取る
int printk(const char* format, ...) {
	va_list ap;
	int result;
	char s[1024];

	va_start(ap, format);
	// vsprintf(): 可変長引数の代わりにva_list型の変数を受け取ることができる
	result = vsprintf(s, format, ap);
	va_end(ap);

	console->PutString(s);
	return result;
}

extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config) {
	// ピクセルのデータ形式frame_buffer_config.pixel_formatに基づいて2つの子クラスの適するインスタンスを生成し、そのインスタンスへのポインタをpixel_writerへ渡す
	// 配置new: メモリの確保は行わないが、引数に指定したメモリ領域の上にインスタンスを生成する。そのメモリ領域に対してコンストラクタを呼び出す
	// 一般のnewはOSがメモリ確保要求を出すようになってはじめて可能である。しかし、ヒープ領域には
	// コンパイラが、operator new(sizeof(RGBResv8BitPerColorPixelWriter), pixel_writer_buf);に展開する
	// そして、コンストラクタを呼び出して、PixelWriterを指すポインタ型であるpixel_writerに渡す
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

	// デスクトップのほぼ全面を青っぽい色で塗りつぶす
	FillRectangle(*pixel_writer,
				  {0, 0},
				  {kFrameWidth, kFrameHeight - 50},
				  kDesktopBGColor);
	// 黒っぽい色で小さな長方形を描く
	FillRectangle(*pixel_writer,
				  {0, kFrameHeight - 50},
				  {kFrameWidth, 50},
				  {1, 8, 17});
	// 灰色で小さな長方形を描く
	FillRectangle(*pixel_writer,
				  {0, kFrameHeight - 50},
				  {kFrameWidth / 5, 50},
				  {80, 80, 80});
	// 明るい灰色で小さな枠を描く
	DrawRectangle(*pixel_writer,
				  {10, kFrameHeight - 50},
				  {30, 30},
				  {160, 160, 160});

	// グローバル領域にConsoleの大きさ分のメモリ領域をcharの配列で確保しておき、配置newでインスタンスを生成する
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

	auto err = pci::ScanAllBus();
	printk("ScanAllBUs: %s\n", err.Name());

	for (int i = 0; i < pci::num_device; ++i) {
		const auto& dev = pci::devices[i];
		auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
		auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
		printk("%d.%d.%d: vend %04x, class %08x, head %02x\n",
			dev.bus, dev.device, dev.function,
			vendor_id, class_code, dev.header_type);
	}

	while (1) __asm__("hlt");
}