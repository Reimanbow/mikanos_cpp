/**
 * @file main.cpp
 * 
 * カーネル本体のプログラムを書いたファイル
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

// 可変長引数を取る
int printk(const char* format, ...) {
	va_list ap;
	int result;
	char s[1024];

	va_start(ap, format);
	// vsprintf(): 可変長引数の代わりにva_list型の変数を受け取ることができる
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

void SwitchEhci2Xhci(const pci::Device& xhc_dev) {
  bool intel_ehc_exist = false;
  for (int i = 0; i < pci::num_device; ++i) {
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) /* EHCI */ &&
        0x8086 == pci::ReadVendorId(pci::devices[i])) {
      intel_ehc_exist = true;
      break;
    }
  }
  if (!intel_ehc_exist) {
    return;
  }

  uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc); // USB3PRM
  pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports); // USB3_PSSEN
  uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4); // XUSB2PRM
  pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports); // XUSB2PR
  Log(kDebug, "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n",
      superspeed_ports, ehci2xhci_ports);
}

usb::xhci::Controller* xhc;

// キューが扱うデータ型となる専用の構造体 
// メッセージの種類を判別するtype値を持つ
struct Message {
	enum Type {
		kInterruptXHCI,
	} type;
};

ArrayQueue<Message>* main_queue;

// 直後に定義される関数が純粋なC++の関数ではなく、割り込みハンドラであることをコンパイラに伝える
__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame* frame) {
	// メッセージ構造体の値を生成してキューにプッシュする
	main_queue->Push(Message{Message::kInterruptXHCI});
	NotifyEndOfInterrupt();
}

extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config,
						   const MemoryMap& memory_map) {
	// ピクセルのデータ形式frame_buffer_config.pixel_formatに基づいて2つの子クラスの適するインスタンスを生成し、そのインスタンスへのポインタをpixel_writerへ渡す
	// 配置new: メモリの確保は行わないが、引数に指定したメモリ領域の上にインスタンスを生成する。そのメモリ領域に対してコンストラクタを呼び出す
	// 一般のnewはOSがメモリ確保要求を出すようになってはじめて可能である。しかし、ヒープ領域には
	// コンパイラが、operator new(sizeof(RGBResv8BitPerColorPixelWriter), pixel_writer_buf);に展開する
	// そして、コンストラクタを呼び出して、PixelWriterを指すポインタ型であるpixel_writerに渡す
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

	// デスクトップのほぼ全面を青っぽい色で塗りつぶす
	FillRectangle(*pixel_writer,
				  {0, 0},
				  {kFrameWidth, kFrameHeight - 50},
				  kDesktopBGColor);
	// 黒っぽい色で小さな長方形を描く
	FillRectangle(*pixel_writer,
				  {0, kFrameHeight - 50},
				  {kFrameWidth, 50},
				  {1, 8, 17});
	// 灰色で小さな長方形を描く
	FillRectangle(*pixel_writer,
				  {0, kFrameHeight - 50},
				  {kFrameWidth / 5, 50},
				  {80, 80, 80});
	// 明るい灰色で小さな枠を描く
	DrawRectangle(*pixel_writer,
				  {10, kFrameHeight - 50},
				  {30, 30},
				  {160, 160, 160});

	// グローバル領域にConsoleの大きさ分のメモリ領域をcharの配列で確保しておき、配置newでインスタンスを生成する
	console = new(console_buf) Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};
	printk("Welcome to MikanOS!\n");

	SetLogLevel(kWarn);

	const std::array available_memory_types {
		MemoryType::kEfiBootServicesCode,
		MemoryType::kEfiBootServicesData,
		MemoryType::kEfiConventionalMemory,
	};

	printk("memory_map: %p\n", &memory_map);
	// メモリマップはメモリディスクリプタの配列であるため、それを順番に表示する
	for (uintptr_t iter = reinterpret_cast<uintptr_t>(memory_map.buffer);
		 iter < reinterpret_cast<uintptr_t>(memory_map.buffer) + memory_map.map_size;
		 iter += memory_map.descriptor_size) {
		auto desc = reinterpret_cast<MemoryDescriptor*>(iter);
		for (int i = 0; i < available_memory_types.size(); ++i) {
			if (desc->type == available_memory_types[i]) {
				printk("type = %u, phys = %08lx - %08lx, pages = %lu, attr = %08lx\n",
					desc->type,
					desc->physical_start,
					desc->physical_start + desc->number_of_pages * 4096 - 1,
					desc->number_of_pages,
					desc->attribute);
			}
		}
	}

  	mouse_cursor = new(mouse_cursor_buf) MouseCursor{
 		pixel_writer, kDesktopBGColor, {300, 200}
	};

	std::array<Message, 32> main_queue_data;
	ArrayQueue<Message> main_queue{main_queue_data};
	::main_queue = &main_queue;

	auto err = pci::ScanAllBus();
	Log(kDebug, "ScanAllBus: %s\n", err.Name());

	for (int i = 0; i < pci::num_device; ++i) {
		const auto& dev = pci::devices[i];
		auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
		auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
		Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n",
			dev.bus, dev.device, dev.function,
			vendor_id, class_code, dev.header_type);
	}

	// Intel製を優先してxHCを探す
	pci::Device* xhc_dev = nullptr;
	for (int i = 0; i < pci::num_device; ++i) {
		if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
			xhc_dev = &pci::devices[i];

			if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
				break;
			}
		}
	}

	if (xhc_dev) {
		Log(kInfo, "xHC has been found: %d.%d.%d\n",
			xhc_dev->bus, xhc_dev->device, xhc_dev->function);
	}

	// セグメントセレクタには、GetCS()で取得した現在のコードセグメントのセレクタ値を指定する
	const uint16_t cs = GetCS();
	// IDTのxHCI用の割り込みベクタに対し、xHCI用の割り込みハンドラを登録する
	SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
				reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);
	// IDTの設定が終わったらIDTの場所をCPUに教える必要がある
	LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

	const uint8_t bsp_local_apic_id =
		*reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;
	// xHCに対してMSI割り込みを有効化するための設定を行う
	// 第2引数がDestination ID、第5引数がVectorフィールドに設定される値
	pci::ConfigureMSIFixedDestination(
		*xhc_dev, bsp_local_apic_id,
		pci::MSITriggerMode::kLevel, pci::MSIDeliveryMode::kFixed,
		InterruptVector::kXHCI, 0
	);

	const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
	Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
	const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
	Log(kDebug, "xHC mmio_base = %08lx\n",xhc_mmio_base);

	usb::xhci::Controller xhc{xhc_mmio_base};

	if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
		SwitchEhci2Xhci(*xhc_dev);
	}
	{
		auto err = xhc.Initialize();
		Log(kDebug, "xhc.Initialize: %s\n", err.Name());
	}

	Log(kInfo, "xHC starting\n");
	xhc.Run();

	::xhc = &xhc;
	__asm__("sti");

	usb::HIDMouseDriver::default_observer = MouseObserver;

  	for (int i = 1; i <= xhc.MaxPorts(); ++i) {
	    auto port = xhc.PortAt(i);
		Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

	    if (port.IsConnected()) {
 	    	if (auto err = ConfigurePort(xhc, port)) {
	        	Log(kError, "failed to configure port: %s at %s:%d\n",
          		  	err.Name(), err.File(), err.Line());
 	    		continue;
      		}
    	}
  	}

	// キューからメッセージを取り出して処理を行う
	while (true) {
		// cli: CPUの割り込みフラグを0にする命令
		// このフラグが0のとき, CPUは外部割り込みを受け取らなくなる. 割り込みハンドラIntHandlerXHCI()は実行されなくなる
		__asm__("cli");
		if (main_queue.Count() == 0) {
			// インラインアセンブラは\n\tで複数の命令を並べることができる
			__asm__("sti\n\thlt");
			continue;
		}

		// キューからメッセージを1つ取り出す
		Message msg = main_queue.Front();
		main_queue.Pop();
		// sti: 割り込みフラグを1にする効果がある
		__asm__("sti");

		// メッセージの内容に応じて処理を行う
		switch (msg.type) {
		case Message::kInterruptXHCI:
			while (xhc.PrimaryEventRing()->HasFront()) {
				if (auto err = ProcessEvent(xhc)) {
					Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
						err.Name(), err.File(), err.Line());
				}
			}
			break;
		default:
			Log(kError, "Unknown message type: %d\n", msg.type);
		}
	}
}

extern "C" void __cxa_pure_virtual() {
	while (1) __asm__("hlt");
}