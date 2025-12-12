/**
 * @file graphics.hpp
 */
#pragma once

#include "frame_buffer_config.hpp"

/**
 * @brief ピクセルの色を設定する構造体
 *
 * RGB各成分を8ビット（0〜255）で表現する
 */
struct PixelColor {
	uint8_t r, g, b;	// [R 8bit][G 8bit][B 8bit]
};

inline bool operator==(const PixelColor& lhs, const PixelColor& rhs) {
	return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

inline bool operator!=(const PixelColor& lhs, const PixelColor& rhs) {
	return !(lhs == rhs);
}

/**
 * @brief ピクセル描画を行う抽象基底クラス
 *
 * フレームバッファへのピクセル描画機能を提供する。
 * ピクセルフォーマット（RGB/BGR）に応じた具体的な描画処理は
 * 派生クラスで実装する。
 */
class PixelWriter {
public:
	/**
	 * @brief 仮想デストラクタ
	 *
	 * 派生クラスのデストラクタが正しく呼ばれるようにする
	 */
	virtual ~PixelWriter() = default;

	/**
	 * @brief 指定座標にピクセルを描画する（純粋仮想関数）
	 * @param x X座標（横方向の位置）
	 * @param y Y座標（縦方向の位置）
	 * @param c 描画する色
	 *
	 * 派生クラスで具体的な描画処理を実装する必要がある
	 */
	virtual void Write(int x, int y, const PixelColor& c) = 0;

	virtual int Width() const = 0;
	virtual int Height() const = 0;
};

class FrameBufferWriter : public PixelWriter {
public:
	FrameBufferWriter(const FrameBufferConfig& config) : config_{config} {
	}
	virtual ~FrameBufferWriter() = default;
	virtual int Width() const override { return config_.horizontal_resolution; }
	virtual int Height() const override { return config_.vertical_resolution; }

protected:
	/**
	 * @brief 指定座標のピクセルのメモリアドレスを取得する
	 * @param x X座標
	 * @param y Y座標
	 * @return ピクセルのメモリアドレス
	 *
	 * 1ピクセル = 4バイト（RGBA or BGRA）
	 * アドレス計算: 先頭 + 4バイト × (1行のピクセル数 × y + x)
	 */
	uint8_t* PixelAt(int x, int y) {
		return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
	}

private:
	const FrameBufferConfig& config_;	// フレームバッファ設定への参照
};

class RGBResv8BitPerColorPixelWriter : public FrameBufferWriter {
public:
	using FrameBufferWriter::FrameBufferWriter;

	virtual void Write(int x, int y, const PixelColor& c) override;
};

class BGRResv8BitPerColorPixelWriter : public FrameBufferWriter {
public:
	using FrameBufferWriter::FrameBufferWriter;

	virtual void Write(int x, int y, const PixelColor& c) override;
};

/**
 * @brief いろいろな型で、2次元ベクトルを表現する構造体
 */
template <typename T>
struct Vector2D {
	T x, y;

	/**
	 * @brief 演算子+=を可能にする
	 * 
	 * Vector2D型の2つの変数a、bに関してa+=bのような操作を可能にする
	 */
	template<typename U>
	Vector2D<T>& operator +=(const Vector2D<U>& rhs) {
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
};

/**
 * @brief 長方形の枠だけ描く
 * 
 * @param writer	PixelWriter型のインスタンスの参照
 * @param pos		長方形の左上の座標
 * @param size		長方形の大きさ
 * @param c			描画色
 */
void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos,
				   const Vector2D<int>& size, const PixelColor& c);

/**
 * @brief 長方形を塗りつぶす
 * 
 * @param writer	PixelWriter型のインスタンスの参照
 * @param pos		長方形の左上の座標
 * @param size		長方形の大きさ
 * @param c			描画色
 */
void FillRectangle(PixelWriter& writer, const Vector2D<int> &pos,
				   const Vector2D<int>& size, const PixelColor& c);

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

void DrawDesktop(PixelWriter& writer);