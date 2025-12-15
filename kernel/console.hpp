/**
 * @file console.hpp
 */
#pragma once

#include <memory>
#include "graphics.hpp"
#include "window.hpp"

class Console {
public:
	static const int kRows = 25, kColumns = 80;

	Console(const PixelColor& fg_color, const PixelColor& bg_color);

	/**
	 * @brief 文字列を出力する
	 * 
	 * @param s 出力する文字列
	 * 
	 * 与えられた文字列を先頭から1文字ずつ処理する
	 * 改行文字ならNewline()に処理を移譲する
	 */
	void PutString(const char* s);

	void SetWriter(PixelWriter* writer);

	void SetWindow(const std::shared_ptr<Window>& window);

	void SetLayerID(unsigned int layer_id);

	unsigned int LayerID() const;

private:
	/**
	 * @brief 改行処理を行う
	 * 
	 * 現在のカーソル位置がまだ最下行に達していないなら単にカーソルを1行進めるだけ
	 * 最下行にあるときはカーソルを進める代わりに表示領域全体を1行ずらすスクロール処理をする必要がある
	 * 1秒ずつbuffer_の中身をずらしながら再描画する
	 */
	void Newline();

	void Refresh();

	PixelWriter* writer_;
	std::shared_ptr<Window> window_;
	const PixelColor fg_color_, bg_color_;
	char buffer_[kRows][kColumns + 1];
	int cursor_row_, cursor_column_;
	unsigned int layer_id_;
};

extern Console* console;

void InitializeConsole();