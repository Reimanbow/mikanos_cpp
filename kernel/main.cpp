/**
 * @file main.cpp
 *
 * カーネル本体
 * MikanOSのカーネル（OS本体）のエントリポイント
 */

#include <cstdint>
#include <cstddef>

#include "frame_buffer_config.hpp"

// Aのフォントデータ
const uint8_t kFontA[16] = {
  0b00000000, //
  0b00011000, //    **
  0b00011000, //    **
  0b00011000, //    **
  0b00011000, //    **
  0b00100100, //   *  *
  0b00100100, //   *  *
  0b00100100, //   *  *
  0b00100100, //   *  *
  0b01111110, //  ******
  0b01000010, //  *    *
  0b01000010, //  *    *
  0b01000010, //  *    *
  0b11100111, // ***  ***
  0b00000000, //
  0b00000000, //	
};

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

	virtual void Write(int x, int y, const PixelColor& c) override {
		auto p = PixelAt(x, y);
		p[0] = c.r;
		p[1] = c.g;
		p[2] = c.b;
	}
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
public:
	using PixelWriter::PixelWriter;

	virtual void Write(int x, int y, const PixelColor& c) override {
		auto p = PixelAt(x, y);
		p[0] = c.b;
		p[1] = c.g;
		p[2] = c.r;
	}
};

/**
 * フォントデータを利用して1文字を描画する. Aのみ
 */
void WriteAscii(PixelWriter &writer, int x, int y, char c, const PixelColor& color) {
	if (c != 'A') {
		return;
	}
	for (int dy = 0; dy < 16; ++dy) {
		for (int dx = 0; dx < 8; ++dx) {
			if ((kFontA[dy] << dx) & 0x80u) {
				writer.Write(x + dx, y + dy, color);
			}
		}
	}
}

/**
 * @brief 配置new
 * 
 * 一般のnew演算子は、OSがメモリを管理できるようになってはじめてできること
 * 引数に指定したメモリ領域上にインスタンスを生成する
 */
void* operator new(size_t size, void* buf) {
	return buf;
}

void operator delete(void* obj) noexcept {
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

/**
 * @brief カーネルのエントリポイント
 *
 * ブートローダー（MikanLoader）から制御が移った後、最初に実行される関数
 *
 * @param frame_buffer_config フレームバッファの情報を格納した構造体の参照
 */
extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config) {
	switch (frame_buffer_config.pixel_format) {
		case kPixelRGBResv8BitPerColor:
			pixel_writer = new(pixel_writer_buf)
				RGBResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
		case kPixelBGRResv8BitPerColor:
			pixel_writer = new(pixel_writer_buf)
				BGRResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
	}

	// 画面全体を白で塗りつぶす
	for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
		for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
			pixel_writer->Write(x, y, {255, 255, 255});
		}
	}

	// 200 x 100の緑の四角形を描画
	for (int x = 0; x < 200; ++x) {
		for (int y = 0; y < 100; ++y) {
			pixel_writer->Write(100 + x, 100 + y, {0, 255, 0});
		}
	}

	WriteAscii(*pixel_writer, 50, 50, 'A', {0, 0, 0});
	WriteAscii(*pixel_writer, 58, 50, 'A', {0, 0, 0});

	// 無限ループでCPUを停止
	// hlt命令でCPUを省電力モードにする（割り込みが来るまで待機）
	while (1) __asm__("hlt");
}