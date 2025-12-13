/**
 * @file timer.cpp
 */
#include "timer.hpp"

namespace {
	const uint32_t kCountMax = 0xffffffffu;
	/**
	 * LVT Timer(Local Vector Table Timer)レジスタ
	 * 主に割り込みに関する設定を行う。Local APICタイマは設定した時間が経過したときに割り込みを起こすことができる
	 * LVT Timerレジスタでは割り込みの許可、不許可と割り込みベクタ番号を設定する
	 * ビット位置	　フィールド名			意味
	 * 0:7			Vector				割り込みベクタの番号
	 * 12			Delivery Status		割り込みの配送状況(0=空き、1配送待ち)
	 * 16			Mask				割り込みマスク(1=割り込み不許可)
	 * 17:18		Timer Mode			タイマ動作モード(0=単発、1=周期)
	 */
	volatile uint32_t& lvt_timer = *reinterpret_cast<uint32_t*>(0xfee00320);
	// カウンタの初期値
	volatile uint32_t& initial_count = *reinterpret_cast<uint32_t*>(0xfee00380);
	// カウンタの現在地
	volatile uint32_t& current_count = *reinterpret_cast<uint32_t*>(0xfee00390);
	/**
	 * Divide Configurationレジスタ
	 * カウンタの減少スピードの設定。このレジスタで分周比を設定できる
	 * 分周はクロックをn分の1にする
	 * Divide Configurationビット3,1,0: 分周比
	 * 000: 2
	 * 001: 4
	 * ...
	 * 110: 128
	 * 111: 1
	 */
	volatile uint32_t& divide_config = *reinterpret_cast<uint32_t*>(0xfee003e0);
}

void InitializeLAPICTimer() {
	// 分周比を1に設定する
	divide_config = 0b1011;
	// 割り込みが発生しないようにする。単発モードにする
	lvt_timer = (0b001 << 16) | 32;
}

void StartLAPICTimer() {
	/**
	 * Initial Countレジスタに値を書き込むことでLocal APICタイマの動作が始まる
	 * 動作が始まると、Initial Countレジスタの値がCurrent Countレジスタにコピーされる
	 * その後1ずつ値が減っていき、0に到達すると動作が止まる
	 * 
	 * なるべく長い時間を図れるように、32ビット幅の最大値である0xffffffffを書き込む
	 */
	initial_count = kCountMax;
}

uint32_t LAPICTimerElapsed() {
	// 0xffffffffと現在値の差を計算し、処理にかかった時間を算出する
	return kCountMax - current_count;
}

void StopLAPICTimer() {
	// タイマの動作中にInitial Countレジスタに0を書くことでタイマの動作を停止させることができる
	initial_count = 0;
}