#!/usr/bin/python3
"""
フォントビットマップをバイナリデータに変換するツール

テキスト形式のフォントビットマップ（.と*で表現）をバイナリ形式に変換し、
C++コードから読み込めるようにする。

入力例（hankaku.txt）:
    .***.....  → 0b11100000 = 0xE0
    *...*....  → 0b10001000 = 0x88
    ...

出力: バイナリファイル（各行が1バイトに変換される）
"""

import argparse
import functools
import re

# ビットマップパターン（.または*または@の連続）を検出する正規表現
BITMAP_PATTERN = re.compile(r'([.*@]+)')


def compile(src: str) -> bytes:
    """
    テキスト形式のフォントビットマップをバイナリデータに変換する

    Args:
        src: フォントビットマップのテキスト（複数行）
             '.' = ピクセルオフ (0)
             '*' または '@' = ピクセルオン (1)

    Returns:
        バイナリデータ（各行が1バイトに変換される）

    Example:
        入力: ".***.\\n*...*"
        出力: b'\\xe0\\x88' (0b11100000, 0b10001000)
    """
    src = src.lstrip()  # 先頭の空白行を削除
    result = []

    for line in src.splitlines():
        # ビットマップパターンにマッチする部分を抽出
        m = BITMAP_PATTERN.match(line)
        if not m:
            continue  # パターンにマッチしない行はスキップ

        # 各文字を0/1のビット列に変換
        # '.' → 0 (ピクセルオフ)
        # '*' または '@' → 1 (ピクセルオン)
        bits = [(0 if x == '.' else 1) for x in m.group(1)]

        # ビット列を整数値に変換（左から順に最上位ビットとして扱う）
        # 例: [1,1,1,0,0,0,0,0] → 0b11100000 = 224
        bits_int = functools.reduce(lambda a, b: 2*a + b, bits)

        # 整数値を1バイトのバイナリデータに変換
        result.append(bits_int.to_bytes(1, byteorder='little'))

    return b''.join(result)


def main():
    """
    コマンドライン引数を解析してフォント変換を実行する

    使い方:
        python makefont.py hankaku.txt -o font.bin
    """
    parser = argparse.ArgumentParser(
        description='Convert text-based font bitmap to binary format'
    )
    parser.add_argument('font', help='path to a font file (text format)')
    parser.add_argument('-o', help='path to an output file', default='font.out')
    ns = parser.parse_args()

    # 入力ファイル（テキスト）を読み込み、バイナリ形式に変換して出力
    with open(ns.o, 'wb') as out, open(ns.font) as font:
        src = font.read()
        out.write(compile(src))


if __name__ == '__main__':
    main()