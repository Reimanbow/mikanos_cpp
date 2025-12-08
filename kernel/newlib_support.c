/**
 * @file newlib_support.c
 * @brief newlibが必要とする低レベル関数（システムコール）を提供
 */
#include <errno.h>
#include <sys/types.h>

/**
 * @brief ヒープ領域を拡張する（mallocが内部で使用）
 *
 * @param incr 拡張するバイト数
 * @return 拡張後のヒープ先頭アドレス（成功時）、(caddr_t)-1（失敗時）
 *
 * 現在の実装は常に失敗を返す。
 * そのため、mallocは使用できないが、sprintfなどスタック上で動作する関数は使える。
 */
caddr_t sbrk(int incr) {
	errno = ENOMEM;
	return (caddr_t)-1;
}

int getpid(void) {
	return 1;
}

int kill(int pid, int sig) {
	errno = EINVAL;
	return -1;
}