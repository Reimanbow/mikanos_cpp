#pragma once

#include "graphics.hpp"

class Console {
public:
	// コンソールの表示領域の大きさを文字列で表したものである
	static const int kRows = 25, kColumns  = 80;

	Console(PixelWriter& writer, const PixelColor& fg_color, const PixelColor& bg_color);
	// クラスのユーザに公開される唯一のインタフェースで、与えられた文字列をコンソールに出力する
	void PutString(const char* s);

private:
	// 改行処理. cursor_column_を0に戻す. cursor_row_は1増やすか、表示領域の最下行に到達したら代わりにスクロール処理を行う
	void Newline();

	PixelWriter& writer_;
	// fg_color_: 文字列を描画する色
	// bg_color_: 表示領域を塗りつぶす色
	const PixelColor fg_color_, bg_color_;
	// コンソール表示領域に表示されている文字列を保存しているバッファ. 行末にヌル文字を書くために列数より1大きい
	char buffer_[kRows][kColumns + 1];
	int cursor_row_, cursor_column_;
};