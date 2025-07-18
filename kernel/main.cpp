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

// 配置new演算子の定義
// operatorキーワードで演算子を定義できる
void* operator new(size_t size, void* buf) {
	return buf;
}

// これがないとリンク時にエラーとなる
void operator delete(void* obj) noexcept {
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

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

	// 白で画面を塗りつぶした後、200x100の緑の四角を描く
	for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
		for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
			pixel_writer->Write(x, y, {255, 255, 255});
		}
	}
	for (int x = 0; x < 200; ++x) {
		for (int y = 0; y < 100; ++y) {
			pixel_writer->Write(x, y, {0, 255, 0});
		}
	}

	Console console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};

	char buf[128];
	for (int i = 0; i < 27; ++i) {
		sprintf(buf, "line %d\n", i);
		console.PutString(buf);
	}

	while (1) __asm__("hlt");
}