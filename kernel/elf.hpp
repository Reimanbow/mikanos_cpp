/**
 * @file elf.hpp
 *
 * ELF（Executable and Linkable Format）ファイル形式の定義
 * カーネルファイル（kernel.elf）を読み込むための構造体と定数
 */

#pragma once

#include <stdint.h>

// ELF64用の型定義
typedef uintptr_t	Elf64_Addr;		// アドレス型
typedef uint64_t 	Elf64_Off;		// ファイルオフセット型
typedef uint16_t	Elf64_Half;		// 16ビット符号なし整数
typedef uint32_t	Elf64_Word;		// 32ビット符号なし整数
typedef int32_t		Elf64_Sword;	// 32ビット符号あり整数
typedef uint64_t	Elf64_Xword;	// 64ビット符号なし整数
typedef int64_t		Elf64_Sxword;	// 64ビット符号あり整数

#define EI_NIDENT 16	// ELF識別子のバイト数

/**
 * @brief 64ビット用ELFのファイルヘッダ
 *
 * ELFファイルの先頭に配置され、ファイル全体の構造を記述する
 */
typedef struct {
	unsigned char 	e_ident[EI_NIDENT];	// ELFの識別情報（マジックナンバーなど）
	Elf64_Half		e_type;				// ファイルタイプ（実行可能、共有ライブラリなど）
	Elf64_Half		e_machine;			// ターゲットアーキテクチャ（x86_64など）
	Elf64_Word		e_version;			// ELFバージョン
	Elf64_Addr		e_entry;			// エントリポイント（プログラムの開始アドレス）
	Elf64_Off		e_phoff;			// プログラムヘッダのファイルオフセット
	Elf64_Off		e_shoff;			// セクションヘッダのファイルオフセット
	Elf64_Word		e_flags;			// プロセッサ固有のフラグ
	Elf64_Half		e_ehsize;			// このヘッダのサイズ
	Elf64_Half		e_phentsize;		// プログラムヘッダの要素1つの大きさ
	Elf64_Half		e_phnum;			// プログラムヘッダの要素数
	Elf64_Half		e_shentsize;		// セクションヘッダの要素1つの大きさ
	Elf64_Half		e_shnum;			// セクションヘッダの要素数
	Elf64_Half		e_shstrndx;			// セクション名文字列テーブルのインデックス
} Elf64_Ehdr;

/**
 * @brief 64ビットELFのプログラムヘッダの要素
 *
 * プログラムをメモリにロードする際のセグメント情報を記述する
 */
typedef struct {
	Elf64_Word	p_type;		// セグメントタイプ（PT_LOAD、PT_DYNAMICなど）
	Elf64_Word	p_flags;	// セグメントフラグ（読み/書き/実行権限）
	Elf64_Off	p_offset;	// ファイル内のオフセット（このセグメントの開始位置）
	Elf64_Addr	p_vaddr;	// 仮想アドレス（メモリ上の配置先）
	Elf64_Addr	p_paddr;	// 物理アドレス（通常は未使用）
	Elf64_Xword	p_filesz;	// ファイル内のサイズ（バイト数）
	Elf64_Xword p_memsz;	// メモリ上のサイズ（p_filesz以上の場合、差分は0埋め）
	Elf64_Xword p_align;	// アライメント（2のべき乗）
} Elf64_Phdr;

// プログラムヘッダタイプの定義
#define PT_NULL		0	// 未使用エントリ
#define PT_LOAD		1	// ロード可能セグメント
#define PT_DYNAMIC	2	// 動的リンク情報
#define PT_INTERP	3	// インタープリタのパス
#define PT_NOTE		4	// 補助情報
#define PT_SHLIB	5	// 予約（未使用）
#define PT_PHDR		6	// プログラムヘッダテーブル自体
#define PT_TLS		7	// スレッドローカルストレージ

/**
 * @brief 動的セクションのエントリ
 *
 * 動的リンク情報を保持する
 */
typedef struct {
	Elf64_Sxword d_tag;		// エントリのタイプ（DT_RELA、DT_NULLなど）
	union {
		Elf64_Xword d_val;	// 整数値
		Elf64_Addr	d_ptr;	// アドレス値
	} d_un;
} Elf64_Dyn;

// 動的セクションタグの定義
#define DT_NULL		0	// 動的セクションの終端マーカー
#define DT_RELA		7	// 再配置テーブルのアドレス
#define DT_RELASZ	8	// 再配置テーブルの合計サイズ
#define DT_RELAENT	9	// 再配置エントリのサイズ

/**
 * @brief 再配置エントリ（addend付き）
 *
 * プログラムロード時にアドレスを修正するための情報
 */
typedef struct {
	Elf64_Addr		r_offset;	// 再配置対象のアドレス
	Elf64_Xword		r_info;		// シンボルインデックスと再配置タイプ
	Elf64_Sxword	r_addend;	// 加算する定数値
} Elf64_Rela;

// 再配置情報の操作マクロ
#define ELF64_R_SYM(i)		((i)>>32)					// シンボルインデックスを取得
#define ELF64_R_TYPE(i)		((i)&0xffffffffL)			// 再配置タイプを取得
#define ELF64_R_INFO(s,t)	(((s)<<32)+(t)&0xffffffffL)	// r_infoを構築

// x86_64用の再配置タイプ
#define R_X86_64_RELATIVE	8	// ベースアドレスからの相対アドレス再配置