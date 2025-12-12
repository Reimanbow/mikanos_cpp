/**
 * @file window.hpp
 */
#pragma once

#include <vector>
#include <optional>
#include "graphics.hpp"

// グラフィックの表示領域を表す
class Window {
public:
	// Windowと関連付けられたPixelWriterを提供する
	class WindowWriter : public PixelWriter {
	public:
		WindowWriter(Window& window) : window_{window} {}

		/**
		 * @brief 指定された位置に色を描く
		 */
		virtual void Write(int x, int y, const PixelColor& c) override {
			window_.At(x, y) = c;
		}
		
		/**
		 * @brief Widthは関連付けられたWindowの横幅をピクセル単位で返す
		 */
		virtual int Width() const override { return window_.Width(); }

		/**
		 * @brief Heightは関連付けられたWindowの高さをピクセル単位で返す
		 */
		virtual int Height() const override { return window_.Height(); }

	private:
		Window& window_;
	};

	/**
	 * @brief 指定されたピクセル数の平面描画領域を作成する
	 */
	Window(int width, int height);
	~Window() = default;
	Window(const Window& rhs) = delete;
	Window& operator=(const Window& rhs) = delete;

	/**
	 * @brief 与えられたPixelWriterにこのウィンドウの表示領域を描画する
	 * 
	 * @param writer	描画先
	 * @param position	writerの左上を基準した描画位置
	 */
	void DrawTo(PixelWriter& writer, Vector2D<int> position);
	
	/**
	 * @brief 透過色を設定する
	 */
	void SetTransparentColor(std::optional<PixelColor> c);

	/**
	 * @brief このインスタンスに紐付いたWindowWriterを取得する
	 */
	WindowWriter* Writer();

	/**
	 * @brief 指定した位置のピクセルを返す
	 */
	PixelColor& At(int x, int y);

	/**
	 * @brief 指定した位置のピクセルを返す
	 */
	const PixelColor& At(int x, int y) const;

	/**
	 * @brief 平面描画領域の横幅をピクセル単位で返す
	 */
	int Width() const;

	/**
	 * @brief 平面描画領域の高さをピクセル単位で返す
	 */
	int Height() const;

private:
	int width_, height_;
	std::vector<std::vector<PixelColor>> data_{};
	WindowWriter writer_{*this};
	std::optional<PixelColor> transparent_color_{std::nullopt};
};