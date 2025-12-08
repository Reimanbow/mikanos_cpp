/**
 * @file pci.cpp
 */
#include "pci.hpp"

#include "asmfunc.h"

namespace {
	using namespace pci;

	uint32_t MakeAddress(uint8_t bus, uint8_t device,
						 uint8_t function, uint8_t reg_addr) {
		// xを左にbits桁だけビットシフトした値を返すラムダ式をshl②セット
		auto shl = [](uint32_t x, unsigned int bits) {
			return x << bits;
		};

		/**
		 * 31		Enableビット. 1にするとCONFIG_DATAの読み書きがPCIコンフィギュレーション空間に転送される
		 * 30:24	予約
		 * 23:16	バス番号(0-255)
		 * 15:11	デバイス番号(0-31)
		 * 10:8		ファンクション番号(0-7)
		 * 7:2		レジスタオフセット(0-255). 4バイト単位のオフセットを指定. コンフィギュレーション空間のどの位置を読みたいか
		 * 1:0		常に0
		 */
		return shl(1, 31)
			| shl(bus, 16)
			| shl(device, 11)
			| shl(function, 8)
			| (reg_addr & 0xfcu);
	}

	/**
	 * @brief devices[nun_devices]に情報を書き込みnum_deviceをインクリメントする
	 * 
	 * @param dev	デバイス
	 */
	Error AddDevice(const Device& device) {
		// 配列が満杯だったら、Error::kFullを返してエラーを伝える
		if (num_device == devices.size()) {
			return MAKE_ERROR(Error::kFull);
		}

		devices[num_device] = device;
		++num_device;
		return MAKE_ERROR(Error::kSuccess);
	}

	/**
	 * @brief 指定されたバスの番号の各デバイスをスキャンする. 有効なデバイスを見つけたらScanDevice()を実行する
	 * 
	 * @param bus バスの番号
	 */
	Error ScanBus(uint8_t bus);

	/**
	 * @brief 指定のファンクションをdevicesに追加する. PCI-PCIブリッジならセカンダリバスに対しScanBus()を実行する
	 */
	Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function) {
		// バス番号、デバイス番号、ファンクション番号の組をdevicesに登録する
		auto class_code = ReadClassCode(bus, device, function);
		auto header_type = ReadHeaderType(bus, device, function);
		Device dev{bus, device, function, header_type, class_code};
		if (auto err = AddDevice(dev)) {
			return err;
		}

		/**
		 * PCI-PCIブリッジだった場合、セカンダリバスの番号を取得しScanBus()を呼び出す
		 * ベースクラスが0x06、サブクラスが0x04ならPCI-PCIブリッジである
		 */
		if (class_code.Match(0x06u, 0x04u)) {
			auto bus_numbers = ReadBusNumbers(bus, device, function);
			uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
			return ScanBus(secondary_bus);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	/**
	 * @brief 指定のデバイス番号の各ファンクションをスキャンする. 有効なファンクションを見つけたらScanFunction()を実行する
	 */
	Error ScanDevice(uint8_t bus, uint8_t device) {
		// まず、ファンクション0をスキャンする
		if (auto err = ScanFunction(bus, device, 0)) {
			return err;
		}
		// シングルファンクションだったら終了する
		if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) {
			return MAKE_ERROR(Error::kSuccess);
		}

		// マルチファンクションの場合は1からスキャンする
		for (uint8_t function = 1; function < 8; ++function) {
			if (ReadVendorId(bus, device, function) == 0xffffu) {
				continue;
			}
			if (auto err = ScanFunction(bus, device, function)) {
				return err;
			}
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error ScanBus(uint8_t bus) {
		// バスは最大32個のデバイスを持つ
		for (uint8_t device = 0; device < 32; ++device) {
			/**
			 * 指定したデバイス番号の位置に実際のデバイスがあるかを調べる
			 * 各デバイスのファンクション0のベンダIDを調べればわかる
			 */
			if (ReadVendorId(bus, device, 0) == 0xffffu) {
				continue;
			}
			if (auto err = ScanDevice(bus, device)) {
				return err;
			}
		}
		return MAKE_ERROR(Error::kSuccess);
	}
};

namespace pci {
	void WriteAddress(uint32_t address) {
		IoOut32(kConfigAddress, address);
	}
	
	void WriteData(uint32_t value) {
		IoOut32(kConfigData, value);
	}

	uint32_t ReadData() {
		return IoIn32(kConfigData);
	}

	uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x00));
		return ReadData() & 0xffffu;
	}

	uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x00));
		return ReadData() >> 16;
	}

	uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x0c));
		return (ReadData() >> 16) & 0xffu;
	}

	ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x08));
		auto reg = ReadData();
		ClassCode cc;
		cc.base			= (reg >> 24) & 0xffu;
		cc.sub			= (reg >> 16) & 0xffu;
		cc.interface	= (reg >> 8)  & 0xffu;
		return cc;
	}
	
	uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x18));
		return ReadData();
	}

	bool IsSingleFunctionDevice(uint8_t header_type) {
		// 8ビット整数のヘッダタイプのビット7が1の場合はマルチファンクションデバイスである
		return (header_type & 0x80u) == 0;
	}

	Error ScanAllBus() {
		num_device = 0;

		/**
		 * 最初にバス0、デバイス0、ファンクション0のPCIコンフィギュレーション空間からヘッダタイプを読み取る
		 * バス0上のデバイス0は必ずホストブリッジである
		 * ホストブリッジはホスト側とPCIバス側を橋渡しする部品で、CPUとPCIデバイスの間の通信はここを必ず通過する
		 */
		auto header_type = ReadHeaderType(0, 0, 0);
		// マルチファンクションデバイスでない場合、そのホストブリッジはバス0を担当するホストブリッジである
		if (IsSingleFunctionDevice(header_type)) {
			return ScanBus(0);
		}

		// マルチファンクションデバイスである場合、ホストブリッジが複数存在する
		for (uint8_t function = 0; function < 8; ++function) {
			if (ReadVendorId(0, 0, function) == 0xffffu) {
				continue;
			}
			if (auto err = ScanBus(function)) {
				return err;
			}
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	uint32_t ReadConfReg(const Device& dev, uint8_t reg_addr) {
		WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
		return ReadData();
	}

	void WriteConfReg(const Device& dev, uint8_t reg_addr, uint32_t value) {
		WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
		WriteData(value);
	}

	WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index) {
		if (bar_index >= 6) {
			return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
		}

		const auto addr = CalcBarAddress(bar_index);
		const auto bar = ReadConfReg(device, addr);

		if ((bar & 4u) == 0) {
			return {bar, MAKE_ERROR(Error::kSuccess)};
		}

		if (bar_index >= 5) {
			return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
		}

		const auto bar_upper = ReadConfReg(device, addr + 4);
		return {
			bar | (static_cast<uint64_t>(bar_upper) << 32),
			MAKE_ERROR(Error::kSuccess)
		};
	}
};