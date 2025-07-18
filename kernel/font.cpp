/**
 * @file font.cpp
 *
 * フォント描画のプログラムを集めたファイル.
 */

#include "font.hpp"

// extern: どこか他のオブジェクトファイルにある変数を参照する(hankaku.o)
extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

// ASCIIコードに対応したフォントデータの先頭アドレスを返す
const uint8_t* GetFont(char c) {
	auto index = 16 * static_cast<unsigned int>(c);
	if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
		return nullptr;
	}
	return &_binary_hankaku_bin_start + index;
}

// 作成したフォントデータを利用して1文字を描画するための関数
void WriteAscii(PixelWriter& writer, int x, int y, char c, const PixelColor& color) {
	const uint8_t* font = GetFont(c);

	// 一番上のピクセルを横方向に描画し、次に2本目のピクセルを横方向に描画し、…を16回繰り返して描画する
	for (int dy = 0; dy < 16; ++dy) {
		for (int dx = 0; dx < 8; ++dx) {
			// フォントデータの該当するビットが1かどうかを検査する
			if ((font[dy] << dx) & 0x80u) {
				writer.Write(x + dx, y + dy, color);
			}
		}
	}
}

// 文字列描画
void WriteString(PixelWriter& writer, int x, int y, const char* s, const PixelColor& color) {
	for (int i = 0; s[i] != '\0'; i++) {
		WriteAscii(writer, x + 8 * i, y, s[i], color);
	}
}