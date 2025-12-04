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
	 * @brief コンストラクタ
	 * @param config フレームバッファの設定情報
	 */
	PixelWriter(const FrameBufferConfig& config) : config_{config} {}

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

class RGBResv8BitPerColorPixelWriter : public PixelWriter {
public:
	using PixelWriter::PixelWriter;

	virtual void Write(int x, int y, const PixelColor& c) override;
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
public:
	using PixelWriter::PixelWriter;

	virtual void Write(int x, int y, const PixelColor& c) override;
};