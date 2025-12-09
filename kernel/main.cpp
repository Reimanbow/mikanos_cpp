/**
 * @file main.cpp
 *
 * カーネル本体
 * MikanOSのカーネル（OS本体）のエントリポイント
 */

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>

#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "graphics.hpp"
#include "mouse.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"
#include "queue.hpp"

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

char console_buf[sizeof(Console)];
Console* console;

int printk(const char* format, ...) {
	va_list ap;
	int result;
	char s[1024];

	va_start(ap, format);
	result = vsprintf(s, format, ap);
	va_end(ap);

	console->PutString(s);
	return result;
}

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor* mouse_cursor;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
	mouse_cursor->MoveRelative({displacement_x, displacement_y});
}

/**
 * @brief USBポートの制御モードを切り替える
 * 
 * パソコンにEHCI準拠でIntel製のコントローラが存在することを確かめる
 * 存在するのであれば、EHCIからxHCIへのモード変更を行う
 */
void SwitchEhci2Xhci(const pci::Device& xhc_dev) {
	bool intel_ehc_exist = false;
	for (int i = 0; i < pci::num_device; ++i) {
		if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) &&
				0x8086 == pci::ReadVendorId(pci::devices[i])) {
			intel_ehc_exist = true;
			break;
		}
	}
	if (!intel_ehc_exist) {
		return;
	}

	uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc);
	pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);
	uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4);
	pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);
	Log(kDebug, "SwitchEhci2Xhci: SS = %02x, xHCI = %02x\n", superspeed_ports, ehci2xhci_ports);
}

usb::xhci::Controller* xhc;

// キューが扱うデータ型
struct Message {
	enum Type {
		kInterruptXHCI,
	} type;
};

ArrayQueue<Message>* main_queue;

/**
 * @brief xHCI用割り込みハンドラ
 * 
 * @note __attribute__((interrupt))により割り込みハンドラとして扱われる
 */
__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame* frame) {
	// キューに対してプッシュする
	main_queue->Push(Message{Message::kInterruptXHCI});
	NotifyEndOfInterrupt();
}

/**
 * @brief カーネルのエントリポイント
 *
 * ブートローダー（MikanLoader）から制御が移った後、最初に実行される関数
 *
 * @param frame_buffer_config	フレームバッファの情報を格納した構造体の参照
 * @param memory_map			メモリマップ
 */
extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config,
						   const MemoryMap& memory_map) {
	switch (frame_buffer_config.pixel_format) {
		case kPixelRGBResv8BitPerColor:
			pixel_writer = new(pixel_writer_buf)
				RGBResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
		case kPixelBGRResv8BitPerColor:
			pixel_writer = new(pixel_writer_buf)
				BGRResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
	}

	const int kFrameWidth = frame_buffer_config.horizontal_resolution;
	const int kFrameHeight = frame_buffer_config.vertical_resolution;

	FillRectangle(*pixel_writer,
				  {0, 0},
				  {kFrameWidth, kFrameHeight},
				  kDesktopBGColor);
	FillRectangle(*pixel_writer,
				  {0, kFrameHeight - 50},
				  {kFrameWidth, 50},
				  {1, 8, 17});
	FillRectangle(*pixel_writer,
				  {0, kFrameHeight - 50},
				  {kFrameWidth / 5, 50},
				  {80, 80, 80});
	DrawRectangle(*pixel_writer,
				  {10, kFrameHeight - 40},
				  {30, 30},
				  {160, 160, 160});

	console = new(console_buf) Console{
		*pixel_writer, kDesktopFGColor, kDesktopBGColor
	};
	printk("Welcome to MikanOS!\n");
	SetLogLevel(kWarn);

	/**
	 * OSが使用可能なメモリ領域のタイプを定義
	 *
	 * BootServicesCode/Data: UEFIブートサービスが使っていた領域（OS起動後は解放可能）
	 * ConventionalMemory: 通常のメモリ（OSが自由に使える）
	 */
	const std::array available_memory_types{
		MemoryType::kEfiBootServicesCode,
		MemoryType::kEfiBootServicesData,
		MemoryType::kEfiConventionalMemory,
	};

	// デバッグ情報: メモリマップのアドレスと基本情報を表示
	printk("memory_map: %p\n", &memory_map);
	printk("Starting loop: buffer=%p, map_size=%lu, desc_size=%lu\n",
	       memory_map.buffer, memory_map.map_size, memory_map.descriptor_size);

	/**
	 * メモリマップを走査して、使用可能なメモリ領域を表示
	 *
	 * - memory_map.bufferの先頭から順番にメモリディスクリプタを読み取る
	 * - 各ディスクリプタはdescriptor_sizeバイトずつ離れている
	 * - 使用可能なタイプ(available_memory_types)のみを表示
	 */
	for (uintptr_t iter = reinterpret_cast<uintptr_t>(memory_map.buffer);
		 iter < reinterpret_cast<uintptr_t>(memory_map.buffer) + memory_map.map_size;
		 iter += memory_map.descriptor_size) {
		// 現在のイテレータ位置をMemoryDescriptor構造体へのポインタとして解釈
		auto desc = reinterpret_cast<MemoryDescriptor*>(iter);

		// 使用可能なメモリタイプかチェック
		for (int i = 0; i < available_memory_types.size(); ++i) {
			if (desc->type == available_memory_types[i]) {
				// メモリ領域の情報を表示
				// phys: 物理アドレス範囲（開始 - 終了）
				// pages: ページ数（1ページ = 4096バイト）
				// attr: メモリ属性
				printk("type = %u, phys = %08lx - %08lx, pages = %lu, attr = %08lx\n",
					desc->type,
					desc->physical_start,
					desc->physical_start + desc->number_of_pages * 4096 - 1,
					desc->number_of_pages,
					desc->attribute);
				break;  // 一致したらinner loopを抜ける
			}
		}
	}

	mouse_cursor = new(mouse_cursor_buf) MouseCursor{
		pixel_writer, kDesktopBGColor, {300, 200}
	};

	std::array<Message, 32> main_queue_data;
	ArrayQueue<Message> main_queue{main_queue_data};
	::main_queue = &main_queue;
	
	// PCIデバイスをすべて検出し、検出されたPCIデバイスの一覧を表示する
	auto err = pci::ScanAllBus();
	Log(kDebug, "ScanAllBus: %s\n", err.Name());

	for (int i = 0; i < pci::num_device; ++i) {
		const auto& dev = pci::devices[i];
		auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
		auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
		Log(kDebug, "%d.%d.%d: vend: %04x, class %08x, head %02x\n",
			dev.bus, dev.device, dev.function,
			vendor_id, class_code, dev.header_type);
	}

	/**
	 * xHCの探索
	 * pci::devicesの中からクラスコードが0x0c、0x03、0x30のものを探す
	 * ベースクラス0x0cはUSBに限らずシリアルバスのコントローラ全体を表す
	 * その中でサブクラス0x03はUSBコントローラを、インタフェース0x30はxHCIを表す
	 */
	pci::Device* xhc_dev = nullptr;
	for (int i = 0; i < pci::num_device; ++i) {
		if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
			xhc_dev = &pci::devices[i];

			// Intel製を優先する. IntelのベンダIDは0x8086
			if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
				break;
			}
		}
	}

	if (xhc_dev) {
		Log(kInfo, "xHC has been found: %d.%d.%d\n",
			xhc_dev->bus, xhc_dev->device, xhc_dev->function);
	}

	// 割り込みベクタ0x40を設定してIDTをCPUに登録する
	const uint16_t cs = GetCS();
	SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
				reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);
	LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

	const uint8_t bsp_local_apic_id =
		*reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;
	pci::ConfigureMSIFixedDestination(
		*xhc_dev, bsp_local_apic_id,
		pci::MSITriggerMode::kLevel, pci::MSIDeliveryMode::kFixed,
		InterruptVector::kXHCI, 0);

	/**
	 * xHCを制御するレジスタ群はMMIOである
	 * MMIOアドレスは、PCIコンフィギュレーション空間にあるBAR0に記録されている
	 */
	const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
	Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
	// 下位4ビットはBARのフラグを表すので、下位4ビットをマスクした値を真のMMIOベースアドレスとする
	const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
	Log(kDebug, "xHC mmio base = %08lx\n", xhc_mmio_base);

	// xHCの初期化と起動
	usb::xhci::Controller xhc{xhc_mmio_base};

	// Intel製のxHCだった場合のみ、xHCの初期化をするためにSwitchEhci2Xhci()を呼び出す
	if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
		SwitchEhci2Xhci(*xhc_dev);
	}
	{
		auto err = xhc.Initialize();
		Log(kDebug, "xhc.Initialize: %s\n", err.Name());
	}

	// xHCの動作を開始させる。動作を開始したxHCは、接続されたUSB機器の認識などを順次進める
	Log(kInfo, "xHC starting\n");
	xhc.Run();

	::xhc = &xhc;
	__asm__("sti");

	/**
	 * 接続されたUSB機器の中からマウスを探す
	 */
	usb::HIDMouseDriver::default_observer = MouseObserver;

	// すべてのUSBポートを探索する
	for (int i = 1; i <= xhc.MaxPorts(); ++i) {
		auto port = xhc.PortAt(i);
		Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

		// port.IsConnected()がtrueならポートには何らかの機器が接続されている
		if (port.IsConnected()) {
			/**
			 * ポートのリセットやxHCの内部設定、クラスドライバの生成などを行う
			 * USBマウスが接続されていたら、usb::HIDMouseDriver::default_observerに設定した関数が
			 * そのUSBマウスからのデータを受信する関数として、USBマウス用のクラスドライバに登録される
			 */
			if (auto err = ConfigurePort(xhc, port)) {
				Log(kError, "failed to configure port: %s at %s:%d\n",
					err.Name(), err.File(), err.Line());
				continue;
			}
		}
	}

	// メッセージを繰り返し処理するイベントループ
	while (true) {
		// 割り込み禁止(CPUが外部割り込みを受け取らなくなる)
		__asm__("cli");
		if (main_queue.Count() == 0) {
			__asm__("sti\n\thlt");
			continue;
		}

		Message msg = main_queue.Front();
		main_queue.Pop();
		// 割り込み許可
		__asm__("sti");

		switch (msg.type) {
		case Message::kInterruptXHCI:
			while (xhc.PrimaryEventRing()->HasFront()) {
				if (auto err = ProcessEvent(xhc)) {
					Log(kError, "Error while ProcessEvnet: %s at %s:%d\n",
						err.Name(), err.File(), err.Line());
				}
			}
			break;
		default:
			Log(kError, "Unknown message type: %d\n", msg.type);
		}
	}

	// 無限ループでCPUを停止
	// hlt命令でCPUを省電力モードにする（割り込みが来るまで待機）
	while (1) __asm__("hlt");
}