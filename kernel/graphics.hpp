#pragma once

#include "frame_buffer_config.hpp"

struct PixelColor {
	uint8_t r, g, b;
};

class PixelWriter {
public:
	// フォントデータの該当するビットが1かどうかを検査する
	PixelWriter(const FrameBufferConfig& config) : config_{config} {
	}
	// ピクセルを描画する
	virtual ~PixelWriter() = default;
	virtual void Write(int x, int y, const PixelColor& c) = 0;

protected:
	uint8_t* PixelAt(int x, int y) {
		return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
	}

private:
	const FrameBufferConfig& config_;
};

class RGBResv8BitPerColorPixelWriter : public PixelWriter {
public:
	// using宣言によって、親クラスのコンストラクタをそのまま子クラスのコンストラクタとして使うことができる
	using PixelWriter::PixelWriter;
	virtual void Write(int x, int y, const PixelColor& c) override;
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
public:
	// using宣言によって、親クラスのコンストラクタをそのまま子クラスのコンストラクタとして使うことができる
	using PixelWriter::PixelWriter;
	virtual void Write(int x, int y, const PixelColor& c) override;
};