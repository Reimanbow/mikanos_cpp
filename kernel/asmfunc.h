/**
 * @file asmfunc.h
 */
#pragma once

#include <stdint.h>

/**
 * IOアドレス空間の読み書きは専用のIO命令が必要なためアセンブリ言語で書き、C++から呼び出す
 */
extern "C" {	
	/**
	 * @brief 引数で指定されたIOポートアドレスに対して32ビット整数を出力する
	 * 
	 * @param addr	IOポートアドレス
	 * @param data	32ビット整数
	 */
	void IoOut32(uint16_t addr, uint32_t data);

	/**
	 * @brief 指定したIOポートアドレスから32ビット整数を入力して返す
	 * 
	 * @param addr	IOポートアドレス
	 */
	uint32_t IoIn32(uint16_t addr);

	/**
	 * @brief 現在のコードセグメントのセレクタ値を返す
	 */
	uint16_t GetCS(void);

	/**
	 * @brief IDTの場所をCPUに教える
	 */
	void LoadIDT(uint16_t limit, uint64_t offset);
}