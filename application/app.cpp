#include "app.hpp"
#include "../utils/libimage/libimage.hpp"
#include "../utils/dirhelper.hpp";

#include <algorithm>
#include <array>

namespace img = libimage;
namespace dir = dirhelper;

namespace app
{


	typedef struct app_state_t
	{		
		dir::file_list_t image_files;
		u32 current_index;

		b32 dir_started = false;
		b32 dir_complete = false;

	} AppState;


	//======= CONFIG =======================

	constexpr u32 IMAGE_WIDTH = BUFFER_WIDTH * 7 / 10;
	constexpr u32 IMAGE_HEIGHT = BUFFER_HEIGHT;
	constexpr u32 IMAGE_X = 0;
	constexpr u32 IMAGE_Y = 0;

	constexpr u32 MAX_IMAGES = 1000;

	constexpr auto IMAGE_EXTENSION = ".png";
	constexpr auto IMAGE_DIR = "D:/test_images/src_fail";


	static void fill_buffer(PixelBuffer& buffer, img::pixel_t const& color)
	{
		auto c = buffer.to_color32(color.red, color.green, color.blue);

		u8* row = (u8*)buffer.memory;
		for (u32 y = 0; y < buffer.height; ++y)
		{
			u32* pixel = (u32*)row;
			for (u32 x = 0; x < buffer.width; ++x)
			{
				*pixel++ = c;
			}

			row += static_cast<size_t>(buffer.width) * buffer.bytes_per_pixel;
		}
	}
	

	static void draw_image(img::image_t const& image, PixelBuffer& buffer, u32 x_begin, u32 y_begin)
	{
		u32 x_end = x_begin + image.width;
		if (x_end > buffer.width)
		{
			x_end = buffer.width;
		}

		u32 y_end = y_begin + image.height;
		if (y_end > buffer.height)
		{
			y_end = buffer.height;
		}

		u32 buffer_pitch = buffer.width * buffer.bytes_per_pixel;
		u32 image_pitch = image.width * sizeof(img::pixel_t);

		//u8* buffer_row = (u8*)buffer.memory + x_begin * buffer.bytes_per_pixel + (y_end - 1) * buffer_pitch;
		u8* buffer_row = (u8*)buffer.memory;
		u8* image_row = (u8*)image.data;

		auto const to_buffer_color = [&](img::pixel_t const& p) { return buffer.to_color32(p.red, p.green, p.blue); };

		for (u32 y = y_begin; y < y_end; ++y)
		{
			u32* buffer_pixel = (u32*)buffer_row;
			auto image_pixel = (img::pixel_t*)image_row;

			for (u32 x = x_begin; x < x_end; ++x)
			{
				*buffer_pixel = to_buffer_color(*image_pixel);

				++buffer_pixel;
				++image_pixel;
			}

			buffer_row += buffer_pitch;
			image_row += image_pitch;
		}
	}

	
	static void load_next_image(AppState& state, PixelBuffer& buffer)
	{
		auto const next = [&](auto const& entry) { return !is_image_file(*entry) && entry != state.dir_end; };

		if (!state.dir_started)
		{
			state.dir_started = true;
			state.current_index = 0;
		}
		else
		{
			++state.current_index;
		}

		if (state.current_index >= state.image_files.size())
		{
			state.dir_complete = true;
			fill_buffer(buffer, img::to_pixel(255, 0, 0));
			return;
		}

		auto current_entry = state.image_files[state.current_index];

		img::image_t loaded_image;
		img::read_image_from_file(current_entry, loaded_image);

		img::image_t resized_image;
		resized_image.width = IMAGE_WIDTH;
		resized_image.height = IMAGE_HEIGHT;
		img::resize_image(loaded_image, resized_image);

		draw_image(resized_image, buffer, IMAGE_X, IMAGE_Y);
	}
	

	static img::view_t buffer_view(PixelBuffer const& buffer)
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


	static void fill_buffer_blue(PixelBuffer& buffer)
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


	static void initialize_memory(ThreadContext& thread, AppMemory& memory, AppState& state)
	{
		state.dir_started = false;
		state.dir_complete = false;

		//auto image_path = fs::current_path();

		state.image_files = dir::get_files_of_type(IMAGE_DIR, IMAGE_EXTENSION, MAX_IMAGES);
	}


	void update_and_render(ThreadContext& thread, AppMemory& memory, Input const& input, PixelBuffer& buffer)
	{
		assert(sizeof(AppState) <= memory.permanent_storage_size);

		auto& state = *(AppState*)memory.permanent_storage;
		if (!memory.is_initialized)
		{
			initialize_memory(thread, memory, state);
			memory.is_initialized = true;
		}

		auto& keyboard = input.keyboard;

		if (keyboard.red.ended_down)
		{
			fill_buffer(buffer, img::to_pixel(255, 0, 0));
		}
		else if (keyboard.green.ended_down)
		{
			fill_buffer(buffer, img::to_pixel(0, 255, 0));
		}
		else if (keyboard.blue.ended_down)
		{
			load_next_image(state, buffer);
		}

		
	}
}