#pragma once

#include <stdint.h>

typedef uintptr_t Elf64_Addr;
typedef uint64_t  Elf64_Off;
typedef uint16_t  Elf64_Half;
typedef uint32_t  Elf64_Word;
typedef int32_t   Elf64_Sword;
typedef uint64_t  Elf64_Xword;
typedef int64_t   Elf64_Sxword;

#define EI_NIDENT 16

// 64ビット用ELFのファイルヘッダの構造
typedef struct {
	unsigned char e_ident[EI_NIDENT];
	Elf64_Half    e_type;
	Elf64_Half    e_machine;
	Elf64_Word    e_version;
	Elf64_Addr    e_entry;
	Elf64_Off     e_phoff;		// プログラムヘッダのファイルオフセットを表す. ここで示されるファイル領域を読めばプログラムヘッダを取得できる
	Elf64_Off     e_shoff;
	Elf64_Word    e_flags;
	Elf64_Half    e_ehsize;
	Elf64_Half    e_phentsize;	// プログラムヘッダの要素1つの大きさを表す. プログラムヘッダは配列になっている
	Elf64_Half    e_phnum;		// プログラムヘッダの要素数を表す
	Elf64_Half    e_shentsize;
	Elf64_Half    e_shnum;
	Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

// 64ビットELFのプログラムヘッダの要素
typedef struct {
	Elf64_Word  p_type;		// PHDR, LOADなどのセグメント種別
	Elf64_Word  p_flags;	// フラグ
	Elf64_Off   p_offset;	// オフセット
	Elf64_Addr  p_vaddr;	// 仮想Addr
	Elf64_Addr  p_paddr;
	Elf64_Xword p_filesz;	// ファイルサイズ
	Elf64_Xword p_memsz;	// メモリサイズ
	Elf64_Xword p_align;
} Elf64_Phdr;

#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6
#define PT_TLS		7

typedef struct {
	Elf64_Sxword d_tag;
	union {
		Elf64_Xword d_val;
		Elf64_Addr  d_ptr;
	} d_un;
} Elf64_Dyn;

#define DT_NULL		0
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT	9

typedef struct {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
	Elf64_Sxword r_addend;
} Elf64_Rela;

#define ELF64_R_SYM(i)	((i) >> 32)
#define ELF64_R_TYPE(i)	((i) & 0xffffffffL)
#define ELF64_R_INFO(s, t)	(((s) << 32) + ((t) & 0xffffffffL))

#define R_X86_64_RELATIVE	8