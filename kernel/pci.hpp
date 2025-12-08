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

	// PCIデバイスのクラスコード
	struct ClassCode {
		uint8_t base, sub, interface;

		/**
		 * @brief ベースクラスが等しい場合に真を返す
		 */
		bool Match(uint8_t b) { return b == base; }
		
		/**
		 * @brief ベースクラスとサブクラスが等しい場合に真を返す
		 */
		bool Match(uint8_t b, uint8_t s) { return Match(b) && s == sub; }

		/**
		 * @brief ベース、サブ、インタフェースが等しい場合に真を返す
		 */
		bool Match(uint8_t b, uint8_t s, uint8_t i) {
			return Match(b, s) && i == interface;
		}
	};

	/**
	 * PCIデバイスを操作するための基礎データを格納する
	 * バス番号、デバイス番号、ファンクション番号はデバイスを特定するのに必須
	 * その他の情報は利便性のため
	 */
	struct Device {
		uint8_t bus, device, function, header_type;
		ClassCode class_code;
	};

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
	 */
	ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);

	inline uint16_t ReadVendorId(const Device& dev) {
		return ReadVendorId(dev.bus, dev.device, dev.function);
	}

	/**
	 * @brief 指定されたPCIデバイスの32ビットレジスタを読み取る
	 * 
	 * @param dev		指定するデバイス
	 * @param reg_addr	読み取りたいレジスタ位置
	 */
	uint32_t ReadConfReg(const Device& dev, uint8_t reg_addr);

	/**
	 * @brief 指定されたPCIデバイスの32ビットレジスタに書き込む
	 * 
	 * @param dev		指定するデバイス
	 * @param reg_addr	書き込みたいレジスタ位置
	 * @param value		書き込む値
	 */
	void WriteConfReg(const Device& dev, uint8_t reg_addr, uint32_t value);

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

	/**
	 * @brief BARxにアクセスするためのアドレスを計算する。BAR0は0x10、BAR1は0x14、、、BAR5は0x24
	 */
	constexpr uint8_t CalcBarAddress(unsigned int bar_index) {
		return 0x10 + 4 * bar_index;
	}

	/**
	 * @brief 連続した2つのBARを読む
	 */
	WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index);
};