#pragma once

#include "app_types.hpp"

//#define DLL_NO_HOTLOAD

namespace app
{
	constexpr auto DLL_FILENAME = "app.dll";
	constexpr auto DLL_COPY_FILENAME = "app_running.dll";

	// allocate memory
	constexpr u32 BUFFER_HEIGHT = 720;
	constexpr u32 BUFFER_WIDTH = BUFFER_HEIGHT * 16 / 9;


	using update_and_render_params = void(AppMemory&, Input const&, PixelBuffer&);
	using update_and_render_f = std::function<update_and_render_params>;

	using end_program_params = void();
	using end_program_f = std::function<end_program_params>;

#ifdef DLL_NO_HOTLOAD

	void update_and_render(AppMemory& memory, Input const& input, PixelBuffer& buffer);	

	void end_program();
	

#endif // DLL_NO_HOTLOAD


	
}