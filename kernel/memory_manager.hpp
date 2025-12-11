/**
 * @file memory_manager.hpp
 */
#pragma once

#include <array>
#include <limits>

#include "error.hpp"

// サイズ単位を使いやすくするためのユーザー定義リテラル
namespace {
	// KiB (キビバイト) 単位: 1 KiB = 1024 バイト
	constexpr unsigned long long operator""_KiB(unsigned long long kib) {
		return kib * 1024;
	}

	// MiB (メビバイト) 単位: 1 MiB = 1024 KiB
	constexpr unsigned long long operator""_MiB(unsigned long long mib) {
		return mib * 1024_KiB;
	}

	// GiB (ギビバイト) 単位: 1 GiB = 1024 MiB
	constexpr unsigned long long operator""_GiB(unsigned long long gib) {
		return gib * 1024_MiB;
	}
}

// 物理メモリフレーム1つの大きさ(バイト)
static const auto kBytesPerFrame{4_KiB};

/**
 * @brief ページフレーム番号を表す型
 *
 * 物理メモリを4KiBのフレームに分割し、各フレームに番号を付けて管理する。
 * このクラスはそのフレーム番号をカプセル化している。
 */
class FrameID {
public:
	explicit FrameID(size_t id) : id_{id} {}
	// フレーム番号を取得
	size_t ID() const { return id_; }
	// フレーム番号から実際の物理メモリアドレスを計算
	void* Frame() const { return reinterpret_cast<void*>(id_ * kBytesPerFrame); }

private:
	size_t id_;
};

// 未定義のページフレーム番号を表す定数としてkNullFrameを定義している
static const FrameID kNullFrame{std::numeric_limits<size_t>::max()};

/**
 * @brief ビットマップ方式でメモリフレームの空き状況を管理するクラス
 *
 * 物理メモリを4KiBのフレームに分割し、各フレームが使用中か空きかを
 * ビットマップで管理する。ビットマップの各ビットは1フレームに対応し、
 * 1なら使用中、0なら空きを表す。
 */
class BitmapMemoryManager {
public:
	// このメモリ管理クラスで扱える最大の物理メモリ量(バイト)
	static const auto kMaxPhysicalMemoryBytes{128_GiB};
	// kMaxPhysicalMemoryBytesまでの物理メモリを扱うために必要なフレーム数
	static const auto kFrameCount{kMaxPhysicalMemoryBytes / kBytesPerFrame};

	// ビットマップ配列の要素型 (1要素で複数フレームを表現)
	using MapLineType = unsigned long;
	// ビットマップ配列の1つの要素のビット数 == 1要素で管理できるフレーム数
	static const size_t kBitsPerMapLine{8 * sizeof(MapLineType)};


	// インスタンスを初期化する
	BitmapMemoryManager();

	/**
	 * @brief 要求されたフレーム数の領域を確保して先頭のフレームIDを返す
	 *
	 * 連続したnum_frames個の空きフレームを探し、見つかったら割り当て済みにする。
	 *
	 * @param num_frames 確保したいフレーム数
	 * @return 確保した領域の先頭フレームIDとエラー情報
	 */
	WithError<FrameID> Allocate(size_t num_frames);

	/**
	 * @brief 指定されたフレーム領域を解放する
	 *
	 * @param start_frame 解放する領域の先頭フレーム
	 * @param num_frames 解放するフレーム数
	 * @return エラー情報
	 */
	Error Free(FrameID start_frame, size_t num_frames);

	/**
	 * @brief 指定されたフレーム領域を割り当て済みとしてマークする
	 *
	 * カーネルなど既に使用中の領域をマークする際に使用。
	 *
	 * @param start_frame マークする領域の先頭フレーム
	 * @param num_frame マークするフレーム数
	 */
	void MarkAllocated(FrameID start_frame, size_t num_frame);

	/**
	 * @brief このメモリマネージャで扱うメモリ範囲を設定する。これ以降、Allocateによるメモリ割り当ては設定された範囲内でのみ行われる
	 *
	 * @param range_begin	メモリ範囲の始点
	 * @param range_end		メモリ範囲の終点。最終フレームの次のフレーム
	 */
	void SetMemoryRange(FrameID range_begin, FrameID range_end);

private:
	// ビットマップ配列。各ビットが1フレームの割り当て状態を表す (1=使用中, 0=空き)
	std::array<MapLineType, kFrameCount / kBitsPerMapLine> alloc_map_;
	// このメモリマネージャで扱うメモリ範囲の始点
	FrameID range_begin_;
	// このメモリマネージャで扱うメモリ範囲の終点。最終フレームの次のフレーム
	FrameID range_end_;

	/**
	 * @brief 指定されたフレームのビットを取得
	 * @return true=割り当て済み, false=空き
	 */
	bool GetBit(FrameID frame) const;

	/**
	 * @brief 指定されたフレームのビットを設定
	 * @param frame 対象フレーム
	 * @param allocated true=割り当て済み, false=空き
	 */
	void SetBit(FrameID frame, bool allocated);
};