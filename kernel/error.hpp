/**
 * @file error.hpp
 * @brief エラーハンドリング用のErrorクラスを提供
 */
#pragma once

#include <cstdio>
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
		kNoEnoughMemory,
		kIndexOutOfRange,
		kHostControllerNotHalted,
		kInvalidSlotID,
		kPortNotConnected,
		kInvalidEndpointNumber,
		kTransferRingNotSet,
		kAlreadyAllocated,
		kNotImplemented,
		kInvalidDescriptor,
		kBufferTooSmall,
		kUnknownDevice,
		kNoCorrespondingSetupStage,
		kTransferFailed,
		kInvalidPhase,
	    kUnknownXHCISpeedID,
   		kNoWaiter,
		kNoPCIMSI,
		kUnknownPixelFormat,
		kLastOfCode, ///< エラーコードの末尾（配列サイズ計算用）
	};

private:
	/// エラーコード名の配列（デバッグ用）
	static constexpr std::array code_names_{
		"kSuccess",
		"kFull",
		"kEmpty",
		"kNoEnoughMemory",
		"kIndexOutOfRange",
		"kHostControllerNotHalted",
		"kInvalidSlotID",
		"kPortNotConnected",
		"kInvalidEndpointNumber",
		"kTransferRingNotSet",
		"kAlreadyAllocated",
		"kNotImplemented",
		"kInvalidDescriptor",
		"kBufferTooSmall",
		"kUnknownDevice",
		"kNoCorrespondingSetupStage",
		"kTransferFailed",
		"kInvalidPhase",
		"kUnknownXHCISpeedID",
		"kNoPCIMSI",
		"kNoWaiter",
		"kUnknownPixelFormat",
	};
	static_assert(Error::Code::kLastOfCode == code_names_.size());

public:
	/**
	 * @brief コンストラクタ
	 * @param code エラーコード
	 * @param file エラーが発生したファイル名（__FILE__マクロで自動設定）
	 * @param line エラーが発生した行番号（__LINE__マクロで自動設定）
	 */
	Error(Code code, const char* file, int line)
		: code_{code}, line_{line}, file_{file} {}

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

	const char* File() const {
		return this->file_;
	}

	int Line() const {
		return this->line_;
	}

private:
	Code code_; ///< 保持しているエラーコード
	int line_;
	const char* file_;
};

#define MAKE_ERROR(code) Error((code), __FILE__, __LINE__)

// エラーを返す可能性のある戻り値を表すための構造体
template <class T>
struct WithError {
	T value;
	Error error;
};