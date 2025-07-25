#pragma once

#include <cstdint>
#include "graphics.hpp"

// 作成したフォントデータを利用して1文字を描画するための関数
void WriteAscii(PixelWriter& writer, int x, int y, char c, const PixelColor& color);
// 文字列描画
void WriteString(PixelWriter& writer, int x, int y, const char* s, const PixelColor& color);