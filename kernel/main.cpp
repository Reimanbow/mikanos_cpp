/**
 * @file main.cpp
 *
 * カーネル本体
 * MikanOSのカーネル（OS本体）のエントリポイント
 */

#include <cstdint>

/**
 * @brief カーネルのエントリポイント
 *
 * ブートローダー（MikanLoader）から制御が移った後、最初に実行される関数
 *
 * @param frame_buffer_base フレームバッファ（画面描画用メモリ領域）の先頭物理アドレス
 *                          この領域に書き込むことで直接画面に描画できる
 *                          例: 0x80000000番地など（GOPから取得した値）
 * @param frame_buffer_size フレームバッファのサイズ（バイト単位）
 *                          画面の解像度とピクセルフォーマットによって決まる
 *                          例: 1920x1080のRGBA形式なら約8MBになる
 */
extern "C" void KernelMain(uint64_t frame_buffer_base,
						   uint64_t frame_buffer_size) {
	// フレームバッファの先頭アドレスをuint8_t*型（1バイト単位でアクセス可能）にキャスト
	uint8_t* frame_buffer = reinterpret_cast<uint8_t*>(frame_buffer_base);

	// フレームバッファ全体を塗りつぶす
	// i % 256により、0〜255が繰り返されるパターンで各バイトを埋める
	for (uint64_t i = 0; i < frame_buffer_size; ++i) {
		frame_buffer[i] = i % 256;
	}

	// 無限ループでCPUを停止
	// hlt命令でCPUを省電力モードにする（割り込みが来るまで待機）
	while (1) __asm__("hlt");
}