/**
 * @file console.cpp
 *
 * コンソール描画のプログラムを集めたファイル．
 */

#include "console.hpp"

#include <cstring>
#include "font.hpp"

Console::Console(PixelWriter& writer,
	const PixelColor& fg_color, const PixelColor& bg_color)
	: writer_{writer}, fg_color_{fg_color}, bg_color_{bg_color},
	  buffer_{}, cursor_row_{0}, cursor_column_{0} {
}

// 与えられた文字列を先頭から1文字ずつ処理する
void Console::PutString(const char* s) {
	while (*s) {
		// 改行文字なら改行
		if (*s == '\n') {
			Newline();
		} else if (cursor_column_ < kColumns - 1) {
			WriteAscii(writer_, 8 * cursor_column_, 16 * cursor_row_, *s, fg_color_);
			buffer_[cursor_row_][cursor_column_] = *s;
			++cursor_column_;
		}
		++s;
	}
}

// 改行処理を行う
void Console::Newline() {
	cursor_column_ = 0;
	// 現在のカーソル位置がまあ最下行に達していないならカーソルを1行進める
	if (cursor_row_ < kRows - 1) {
		++cursor_row_;
	// カーソルが最下行にあるときは表示領域全体を1行ずらすスクロール処理をする
	} else {
		for (int y = 0; y < 16 * kRows; ++y) {
			for (int x = 0; x < 8 * kColumns; ++x) {
				writer_.Write(x, y, bg_color_);
			}
		}
		// row + 1行目をrow行目にコピーする
		for (int row = 0; row < kRows - 1; ++row) {
			memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
			WriteString(writer_, 0, 16 * row, buffer_[row], fg_color_);
		}
		// 指定した配列を指定した値で埋める
		memset(buffer_[kRows - 1], 0, kColumns + 1);
	}
}