/**
 * @file libcxx_support.cpp
 * @brief libc++（C++標準ライブラリ）が必要とする低レベル関数を提供
 *
 * newlib_support.cのC++版。libc++がリンク時に要求する関数のダミー実装を提供する。
 */
#include <new>
#include <cerrno>

std::new_handler std::get_new_handler() noexcept {
	return nullptr;
}

extern "C" int posix_memalign(void**, size_t, size_t) {
	return ENOMEM;
}

extern "C" void __cxa_pure_virtual() {
	while (1) __asm__("hlt");
}