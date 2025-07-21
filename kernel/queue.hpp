#pragma once

#include <cstddef>
#include <array>

#include "error.hpp"

// テンプレートクラスとするための記述
// この記述のおかげで、クラス内部ではTを何らかのデータ型を表す文字として扱えるようになる
template <typename T>
class ArrayQueue {
public:
	template<size_t N>
	ArrayQueue(std::array<T, N>& buf);
	ArrayQueue(T* buf, size_t size);
	Error Push(const T& value);
	Error Pop();
	size_t Count() const;
	size_t Capacity() const;
	const T& Front() const;

private:
	// キューに格納されるデータを実際に保管するための配列
	T* data_;
	/**
	 * read_pos_: キューに格納されている先頭のデータを指す。ポップされる対象のデータでFront()で取得できる
	 * write_pos_: キューに格納される末尾のデータの直後の空き領域の先頭を指す。プッシュによってデータが書かれる場所
	 * count_: 格納されている要素数
	 */
	size_t read_pos_, write_pos_, count_;

	// 配列の要素数(固定長)
	const size_t capacity_;
};

// 2引数コンストラクタに処理を移譲する
// 唯一の引数bufから、先頭ポインタと要素数という2つの情報を得て2引数コンストラクタに渡している
template <typename T>
template <size_t N>
ArrayQueue<T>::ArrayQueue(std::array<T, N>& buf) : ArrayQueue(buf.data(), N) {}

// bufはキューのデータを格納する場所となる配列を指定する。sizeはその配列の要素数を指定する
// data_とcapacity_を初期化している
template <typename T>
ArrayQueue<T>::ArrayQueue(T* buf, size_t size)
	: data_{buf}, read_pos_{0}, write_pos_{0}, count_{0}, capacity_{size}
{}

// キューにデータをプッシュする
template <typename T>
Error ArrayQueue<T>::Push(const T& value) {
	// キューが満杯であればエラーError::kFullを返して終了する
	if (count_ == capacity_) {
		return MAKE_ERROR(Error::kFull);
	}

	// 引数で与えられた値valueをwrite_pos_が指す位置に書き込み、write_pos_をインクリメントする
	data_[write_pos_] = value;
	++count_;
	++write_pos_;
	// write_pos_が配列の末尾を超えたら0にリセットする
	// この動作によって配列の末尾と先頭が繋がり、リングのように扱える
	if (write_pos_ == capacity_) {
		write_pos_ = 0;
	}
	return MAKE_ERROR(Error::kSuccess);
}

// キューからデータをポップする関数
// Pop()はポップされた値を返さない
template <typename T>
Error ArrayQueue<T>::Pop() {
	// キューが空であればエラーError::kEmptyを返して終了する
	if (count_ == 0) {
		return MAKE_ERROR(Error::kEmpty);
	}

	// 先頭要素を削除する
	--count_;
	++read_pos_;
	// read_pos_をインクリメントした結果、配列の末尾を超えたら0にリセットする
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

// キューの戦闘データを得る
// constでメンバ変数の値を変化させないことを表す。Front()を呼び出すだけならキューの中身が変化しない
template <typename T>
const T& ArrayQueue<T>::Front() const {
	return data_[read_pos_];
}