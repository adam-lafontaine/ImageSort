#include "app.hpp"
#include "../utils/libimage/libimage.hpp"

#include <algorithm>

namespace img = libimage;

namespace app
{
	InternalFunction void fill_buffer_red(PixelBuffer& buffer)
	{
		auto red = img::to_pixel(255, 0, 0);

		auto c = buffer.to_color32(red.red, red.green, red.blue);

		u8* row = (u8*)buffer.memory;
		for (u32 y = 0; y < buffer.height; ++y)
		{
			u32* pixel = (u32*)row;
			for (u32 x = 0; x < buffer.width; ++x)
			{
				*pixel++ = c;
			}

			row += buffer.width * buffer.bytes_per_pixel;
		}
	}


	InternalFunction img::view_t buffer_view(PixelBuffer const& buffer)
	{
		img::view_t view;

		view.image_data = (img::pixel_t*)buffer.memory;
		view.image_width = buffer.width;
		view.width = buffer.width;
		view.height = buffer.height;
		view.x_begin = 0;
		view.x_end = buffer.width;
		view.y_begin = 0;
		view.y_end = buffer.height;

		return view;
	}


	InternalFunction void fill_buffer_blue(PixelBuffer& buffer)
	{
		auto blue = img::to_pixel(0, 0, 255);

		auto view = buffer_view(buffer);

		auto to_buffer_pixel = [&](img::pixel_t const& p) 
		{
			img::pixel_t c;
			c.value = buffer.to_color32(p.red, p.green, p.blue);
			return c;
		};

		auto c = to_buffer_pixel(blue);

		std::fill(view.begin(), view.end(), c);
	}


	void update_and_render(ThreadContext& thread, MemoryState& memory, Input const& input, PixelBuffer& buffer)
	{
		fill_buffer_blue(buffer);
	}
}