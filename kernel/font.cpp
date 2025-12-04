/**
 * @file font.cpp
 * @brief フォントデータを管理し、文字描画機能を提供する
 */
#include "font.hpp"

/**
 * objcopyで生成されたシンボル（hankaku.bin → hankaku.o）
 *
 * これらのシンボルは実際にはメモリアドレスを表す。
 * hankaku.binの全バイトデータは.dataセクションに配置され、
 * 以下のシンボルがそのアドレスを指す：
 *
 * - _binary_hankaku_bin_start: .dataセクションの先頭アドレス（フォントデータの開始位置）
 * - _binary_hankaku_bin_end:   .dataセクションの終端アドレス（フォントデータの終了位置）
 * - _binary_hankaku_bin_size:  データサイズ（end - start）
 *
 * 注意：これらは変数ではなく、リンカが解決する「アドレスを持つシンボル」である。
 *       &_binary_hankaku_bin_start で実際のデータ領域の先頭アドレスを取得できる。
 */
extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

/**
 * @brief ASCIIコードに対応したフォントデータの先頭アドレスを返す
 *
 * @param c ASCII文字コード（0x00〜0xFF）
 * @return フォントデータの先頭アドレス（16バイト分）、範囲外の場合はnullptr
 *
 * フォントデータの構造：
 * - 各文字は 16バイト（縦16ピクセル × 横8ピクセル）
 * - hankaku.binには 0x00〜0xFF の256文字分が順番に格納されている
 * - index = 文字コード × 16 で、その文字のフォントデータ先頭位置を計算
 *
 * 例：文字 'A' (0x41) の場合
 *   → index = 0x41 × 16 = 0x410 バイト目から16バイトがフォントデータ
 */
const uint8_t* GetFont(char c) {
    // 文字コードを16倍してバイトオフセットを計算
    // （1文字 = 16バイトなので）
    auto index = 16 * static_cast<unsigned int>(c);

    // フォントデータの範囲外チェック
    // _binary_hankaku_bin_sizeはデータサイズを表すシンボル
    // そのアドレス（＝サイズの数値）と比較している
    if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
      return nullptr;
    }

    // フォントデータの先頭アドレス + オフセット を返す
    return &_binary_hankaku_bin_start + index;
}

void WriteAscii(PixelWriter &writer, int x, int y, char c, const PixelColor& color) {
    const uint8_t* font = GetFont(c);
    if (font == nullptr) {
        return;
    }
	  for (int dy = 0; dy < 16; ++dy) {
		    for (int dx = 0; dx < 8; ++dx) {
			      if ((font[dy] << dx) & 0x80u) {
				        writer.Write(x + dx, y + dy, color);
			      }
		    }
	  }
}

void WriteString(PixelWriter& writer, int x, int y, const char* s, const PixelColor& color) {
    for (int i = 0; s[i] != '\0'; ++i) {
      WriteAscii(writer, x + 8 * i, y, s[i], color);
    }
}