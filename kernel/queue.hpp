/**
 * @file queue.hpp
 */
#pragma once

#include <cstddef>
#include <array>

#include "error.hpp"

template <typename T>
class ArrayQueue {
public:
	template <size_t N>
	ArrayQueue(std::array<T, N>& buf);
	ArrayQueue(T* buf, size_t size);

	/**
	 * @brief データをプッシュする
	 */
	Error Push(const T& value);

	/**
	 * @brief データをポップする。ただし、値を返さない
	 */
	Error Pop();

	/**
	 * @brief 利用可能な要素数を取得する
	 */
	size_t Count() const;

	/**
	 * @brief 配列の要素数を取得する
	 */
	size_t Capacity() const;

	/**
	 * @brief 先頭のデータ(read_pos_)を取得する
	 */
	const T& Front() const;

private:
	// キューに格納されるデータを実際に保管するための配列
	T* data_;
	/**
	 * read_pos_はキューに格納されている先頭のデータを指す
	 * write_pos_はキューに格納されている末尾のデータの直後(空き領域)を指す
	 * count_は利用可能な要素数
	 */
	size_t read_pos_, write_pos_, count_;

	// 配列の要素数
	const size_t capacity_;
};

template <typename T>
template <size_t N>
ArrayQueue<T>::ArrayQueue(std::array<T, N>& buf) : ArrayQueue(buf.data(), N) {}

template<typename T>
ArrayQueue<T>::ArrayQueue(T* buf, size_t size)
	: data_{buf}, read_pos_{0}, write_pos_{0}, count_{0}, capacity_{size}
{}

template <typename T>
Error ArrayQueue<T>::Push(const T& value) {
	// キューが満杯ならエラー
	if (count_ == capacity_) {
		return MAKE_ERROR(Error::kFull);
	}

	data_[write_pos_] = value;
	++count_;
	++write_pos_;
	// write_pos_が配列の末尾なら0にリセット
	if (write_pos_ == capacity_) {
		write_pos_ = 0;
	}
	return MAKE_ERROR(Error::kSuccess);
}

template <typename T>
Error ArrayQueue<T>::Pop() {
	// キューが空ならエラー
	if (count_ == 0) {
		return MAKE_ERROR(Error::kEmpty);
	}

	--count_;
	++read_pos_;
	// read_pos_が配列の末尾なら0にリセット
	if (read_pos_ == capacity_) {
		read_pos_ = 0;
	}
	return MAKE_ERROR(Error::kSuccess);
}

template <typename T>
size_t ArrayQueue<T>::Count() const {
	return count_;
}

template <typename T>
size_t ArrayQueue<T>::Capacity() const {
	return capacity_;
}

template <typename T>
const T& ArrayQueue<T>::Front() const {
	return data_[read_pos_];
}