#pragma once

#include "app_types.hpp"

#define DLL_NO_HOTLOAD

namespace app
{
	constexpr auto DLL_FILENAME = "app.dll";
	constexpr auto DLL_COPY_FILENAME = "app_running.dll";

	// allocate memory
	constexpr u32 BUFFER_HEIGHT = 720;
	constexpr u32 BUFFER_WIDTH = BUFFER_HEIGHT * 16 / 9;


#ifdef DLL_NO_HOTLOAD

	void update_and_render(AppMemory& memory, Input const& input, PixelBuffer& buffer);
	using update_and_render_f = std::function<void(AppMemory&, Input const&, PixelBuffer&)>;

	void end_program();
	using end_program_f = std::function<void()>;

#endif // DLL_NO_HOTLOAD


	
}