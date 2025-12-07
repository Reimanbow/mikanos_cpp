/**
 * @file error.hpp
 * @brief エラーハンドリング用のErrorクラスを提供
 */
#pragma once

#include <array>

/**
 * @brief エラーコードをラップするクラス
 *
 * 使用例：
 *   Error result = SomeFunction();
 *   if (result) {  // operator bool() が呼ばれる
 *       // エラーが発生した場合の処理
 *       printf("Error: %s\n", result.Name());
 *   }
 */
class Error {
public:
	/**
	 * @brief エラーコード定義
	 */
	enum Code {
		kSuccess,    ///< 成功
		kFull,       ///< バッファ/コンテナが満杯
		kEmpty,      ///< バッファ/コンテナが空
		kLastOfCode, ///< エラーコードの末尾（配列サイズ計算用）
	};

	/**
	 * @brief コンストラクタ
	 * @param code エラーコード
	 */
	Error(Code code) : code_{code} {}

	/**
	 * @brief bool型への変換演算子
	 * @return true: エラーあり、false: 成功
	 *
	 * if文での条件判定に使用できる。
	 * kSuccess以外の場合にtrueを返す。
	 */
	operator bool() const {
		return this->code_ != kSuccess;
	}

	/**
	 * @brief エラーコード名を取得
	 * @return エラーコード名の文字列
	 */
	const char* Name() const {
		return code_names_[static_cast<int>(this->code_)];
	}

private:
	/// エラーコード名の配列（デバッグ用）
	static constexpr std::array<const char*, 3> code_names_ = {
		"kSuccess",
		"kFull",
		"kEmpty",
	};

	Code code_; ///< 保持しているエラーコード
};