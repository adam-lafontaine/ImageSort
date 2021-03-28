#pragma once

#include "app_types.hpp"

#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))
#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

#define GlobalVariable static
#define InternalFunction static

namespace app
{
	constexpr u32 BUFFER_HEIGHT = 1000;
	constexpr u32 BUFFER_WIDTH = BUFFER_HEIGHT * 16 / 9;
	


	void update_and_render(ThreadContext& thread, MemoryState& memory, Input const& input, PixelBuffer& buffer);
}