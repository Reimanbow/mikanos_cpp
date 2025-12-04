/**
 * @file font.hpp
 * @brief フォント描画機能を提供する
 */
#pragma once

#include <cstdint>
#include "graphics.hpp"

/**
 * @brief フォントデータを利用して1文字を描画する
 *
 * @param writer ピクセル描画を行うPixelWriterオブジェクト
 * @param x 描画開始X座標（左上）
 * @param y 描画開始Y座標（左上）
 * @param c 描画する文字（ASCII文字）
 * @param color 描画色
 *
 * GetFont()で取得したフォントデータ（16バイト）を解釈し、
 * ビットが立っているピクセルを指定色で描画する。
 * フォントは 8x16 ピクセルのビットマップ形式。
 */
void WriteAscii(PixelWriter& writer, int x, int y, char c, const PixelColor& color);

/**
 * @brief 文字列を描画する
 *
 * @param writer ピクセル描画を行うPixelWriterオブジェクト
 * @param x 描画開始X座標（左上）
 * @param y 描画開始Y座標（左上）
 * @param s 描画する文字列（NULL終端）
 * @param color 描画色
 *
 * 文字列の各文字をWriteAscii()で順番に描画する。
 * 各文字は横8ピクセルずつ右にずらして配置される。
 */
void WriteString(PixelWriter& writer, int x, int y, const char* s, const PixelColor& color);