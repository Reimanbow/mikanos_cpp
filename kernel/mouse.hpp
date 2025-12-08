/**
 * @file mouse.hpp
 */
#pragma once

#include "graphics.hpp"

// マウスカーソルの描画と移動に責任を持つクラス
class MouseCursor {
public:
	MouseCursor(PixelWriter* writer, PixelColor erase_color, Vector2D<int> initial_position);

	/**
	 * @brief マウスカーソルを指定された方向に移動させる
	 * 
	 * @param displacement マウスカーソルが移動した分
	 */
	void MoveRelative(Vector2D<int> displacement);

private:
	PixelWriter* pixel_writer_ = nullptr;
	PixelColor erase_color_;
	Vector2D<int> position_;
};