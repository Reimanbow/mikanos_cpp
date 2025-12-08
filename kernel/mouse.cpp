/**
 * @file mouse.cpp
 */
#include "mouse.hpp"

#include "graphics.hpp"

/**
 * マウスカーソルの形の定義
 * '@'がマウスカーソルの縁、'.'がマウスカーソルの内側
 * 配列の横方向は文字列末尾のヌル文字を格納するため
 */
namespace {
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

	/**
	 * @brief 指定した位置にマウスカーソルを描画する
	 */
	void DrawMouseCursor(PixelWriter *pixel_writer, Vector2D<int> position) {
		for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
			for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
				if (mouse_cursor_shape[dy][dx] == '@') {
					pixel_writer->Write(position.x + dx, position.y + dy, {0, 0, 0});
				} else if (mouse_cursor_shape[dy][dx] == '.') {
					pixel_writer->Write(position.x + dx, position.y + dy, {255, 255, 255});
				}
			}
		}
	}

	/**
	 * @brief マウス部分を指定した色で塗りつぶし消す
	 */
	void EraseMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position, PixelColor erase_color) {
		for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
			for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
				if (mouse_cursor_shape[dy][dx] != ' ') {
					pixel_writer->Write(position.x + dx, position.y + dy, erase_color);
				}
			}
		}
	}
}

MouseCursor::MouseCursor(PixelWriter* writer, PixelColor erase_color,
						 Vector2D<int> initial_position)
	: pixel_writer_{writer},
	  erase_color_{erase_color},
	  position_{initial_position} {
	DrawMouseCursor(pixel_writer_, position_);
}

void MouseCursor::MoveRelative(Vector2D<int> displacement) {
	EraseMouseCursor(pixel_writer_, position_, erase_color_);
	position_ += displacement;
	DrawMouseCursor(pixel_writer_, position_);
}