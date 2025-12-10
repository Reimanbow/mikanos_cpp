/**
 * @file x86_descriptor.hpp
 */
#pragma once

enum class DescriptorType {
	// システムセグメント
	kUpper8Bytes	= 0,
	kLDT			= 2,
	kTSSAvailable	= 9,
	kTSSBusy		= 11,
	kCallGate		= 12,
	kInterruptGate	= 14,
	kTrapGate		= 15,

	// コード、データセグメント
	kReadWrite		= 2,
	kExecuteRead	= 10,
};