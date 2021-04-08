#pragma once

#include "app_types.hpp"




namespace app
{
	// allocate memory
	constexpr u32 BUFFER_HEIGHT = 720;
	constexpr u32 BUFFER_WIDTH = BUFFER_HEIGHT * 16 / 9;

	void update_and_render(AppMemory& memory, Input const& input, PixelBuffer& bufferr);

	void end_program();
}