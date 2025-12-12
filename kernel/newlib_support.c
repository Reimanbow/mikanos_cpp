/**
 * @file newlib_support.c
 * @brief newlibが必要とする低レベル関数（システムコール）を提供
 */
#include <errno.h>
#include <sys/types.h>

void _exit(void) {
	while (1) __asm__("hlt");
}

// プログラムブレークの初期値と末尾を表す変数。sbrkを初めて使う前にこれら2つの変数を初期化する必要がある
caddr_t program_break, program_break_end;

/**
 * @brief ヒープ領域を拡張する（mallocが内部で使用）
 *
 * @param incr 拡張するバイト数
 * @return 拡張後のヒープ先頭アドレス（成功時）、(caddr_t)-1（失敗時）
 * 
 * sbrk()はプログラムブレークをincrバイトだけを増減させる
 * 処理が成功したら、増加させる前のプログラムブレークを返す
 * 処理が失敗したらerrnoをENOMEMに設定し、(caddr_t)-1を返す
 * メモリマネージャを利用して割り当てたメモリ領域を利用する
 */
caddr_t sbrk(int incr) {
	// program_breakが設定されていること、メモリ領域が十分に空いていることを確認する
	if (program_break == 0 || program_break + incr >= program_break_end) {
		errno = ENOMEM;
		return (caddr_t)-1;
	}

	caddr_t prev_break = program_break;
	program_break += incr;
	return prev_break;
}

int getpid(void) {
	return 1;
}

int kill(int pid, int sig) {
	errno = EINVAL;
	return -1;
}