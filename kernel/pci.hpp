/**
 * @file pci.hpp
 */
#pragma once

#include <cstdint>
#include <array>

#include "error.hpp"

namespace pci {
	// CONFIG_ADDRESSレジスタのIOポートアドレス
	const uint16_t kConfigAddress = 0x0cf8;
	// CONFIG_DATAレジスタのIOポートアドレス
	const uint16_t kConfigData = 0x0cfc;

	/**
	 * @brief CONFIG_ADDRESSに指定された整数を書き込む
	 * 
	 * @param address CONFIG_ADDRESSレジスタに書き込む32ビット値
	 */
	void WriteAddress(uint32_t address);

	/**
	 * @brief CONFIG_DATAに指定された整数を書き込む
	 * 
	 * @param value CONFIG_DATAレジスタに書き込む32ビット値
	 */
	void WriteData(uint32_t value);

	/**
	 * @brief CONFIG_DATAから32ビット整数を読み込む
	 */
	uint32_t ReadData();

	/**
	 * @brief ベンダIDレジスタを読み取る
	 * 
	 * @param bus 		バス番号
	 * @param device 	デバイス番号
	 * @param function	ファンクション番号
	 */
	uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);

	/**
	 * @brief デバイスIDレジスタを読み取る
	 * 
	 * @param bus 		バス番号
	 * @param device 	デバイス番号
	 * @param function	ファンクション番号
	 */
	uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);

	/**
	 * @brief ヘッダタイプレジスタを読み取る
	 * 
	 * @param bus 		バス番号
	 * @param device 	デバイス番号
	 * @param function	ファンクション番号
	 */
	uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);

	/**
	 * @brief クラスコードレジスタを読み取る
	 * 
	 * @param bus 		バス番号
	 * @param device 	デバイス番号
	 * @param function	ファンクション番号
	 * 
	 * 返される32ビット整数の構造
	 * - 31:24	ベースクラス
	 * - 23-16	サブクラス
	 * - 15:8	インタフェース
	 * - 7:0	リビジョン
	 */
	uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);

	/**
	 * @brief バス番号レジスタを読み取る
	 * 
	 * @param bus 		バス番号
	 * @param device 	デバイス番号
	 * @param function	ファンクション番号
	 * 
	 * 返される32ビット整数の構造
	 * - 23:16	サブオーディネイトバス番号
	 * - 15:8	セカンダリバス番号
	 * - 7:0	リビジョン番号
	 */
	uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

	/**
	 * @brief 単一ファンクションの場合に真を返す
	 */
	bool IsSingleFunctionDevice(uint8_t header_type);

	/**
	 * @brief PCIデバイスを操作するための基礎データを格納する
	 */
	struct Device {
		uint8_t bus, device, function, header_type;
	};

	// ScanAllBus()により発見されたPCIデバイスの一覧
	inline std::array<Device, 32> devices;
	// devicesの有効な要素の数
	inline int num_device;

	/**
	 * @brief PCIデバイスをすべて探索しdevicesに格納する
	 * 
	 * バス0から再帰的にPCIデバイスを探索し、devicesの先頭から詰めて書き込む
	 * 発見したデバイスの数をnum_deviceに設定する
	 */
	Error ScanAllBus();
};