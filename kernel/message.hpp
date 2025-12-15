/**
 * @file message.hpp
 */
#pragma once

struct Message {
	enum Type {
		kInterruptXHCI,
	} type;
};