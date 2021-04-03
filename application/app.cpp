#include "app.hpp"
#include "../utils/libimage/libimage.hpp"
#include "../utils/dirhelper.hpp"

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

		b32 app_started = false;

	} AppState;


	//======= CONFIG =======================

	constexpr u32 IMAGE_WIDTH = BUFFER_WIDTH * 7 / 10;
	constexpr u32 IMAGE_HEIGHT = BUFFER_HEIGHT;

	constexpr img::pixel_range_t IMAGE_RANGE = { 0, IMAGE_WIDTH, 0, IMAGE_HEIGHT };

	constexpr u32 CLASS_SELECT_WIDTH = BUFFER_WIDTH - IMAGE_WIDTH;
	constexpr u32 CLASS_SELECT_HEIGHT = IMAGE_HEIGHT / 2;

	constexpr img::pixel_range_t PASS_RANGE = { IMAGE_WIDTH, BUFFER_WIDTH, 0, CLASS_SELECT_HEIGHT };
	constexpr img::pixel_range_t FAIL_RANGE = { IMAGE_WIDTH, BUFFER_WIDTH, CLASS_SELECT_HEIGHT, BUFFER_HEIGHT };

	
	constexpr u32 IMAGE_X = 0;
	constexpr u32 IMAGE_Y = 0;

	constexpr u32 MAX_IMAGES = 1000;

	constexpr auto IMAGE_EXTENSION = ".png";
	constexpr auto IMAGE_DIR = "D:/test_images/src_fail";
	constexpr auto PASS_DIR = "D:/test_images/sorted_pass";
	constexpr auto FAIL_DIR = "D:/test_images/sorted_fail";


	static b32 in_range(u32 x, u32 y, img::pixel_range_t const& range)
	{
		return x >= range.x_begin && x < range.x_end&& y >= range.y_begin && y < range.y_end;
	}


	static void fill_buffer(PixelBuffer& buffer, img::pixel_t const& color)
	{
		auto c = buffer.to_color32(color.red, color.green, color.blue);

		auto buffer_pitch = static_cast<size_t>(buffer.width) * buffer.bytes_per_pixel;

		u8* row = (u8*)buffer.memory;
		for (u32 y = 0; y < buffer.height; ++y)
		{
			u32* pixel = (u32*)row;
			for (u32 x = 0; x < buffer.width; ++x)
			{
				*pixel++ = c;
			}

			row += buffer_pitch;
		}
	}


	static void draw_rect(PixelBuffer& buffer, img::pixel_range_t const& range, img::pixel_t const& color)
	{
		assert(range.x_end > range.x_begin);
		assert(range.y_end > range.y_begin);

		if (range.x_begin >= buffer.width || range.y_begin >= buffer.height)
		{
			return;
		}

		u32 x_begin = range.x_begin;
		u32 y_begin = range.y_begin;

		u32 x_end = range.x_end;
		if (x_end > buffer.width)
		{
			x_end = buffer.width;
		}

		u32 y_end = range.y_end;
		if (y_end > buffer.height)
		{
			y_end = buffer.height;
		}

		auto c = buffer.to_color32(color.red, color.green, color.blue);

		auto buffer_pitch = static_cast<size_t>(buffer.width) * buffer.bytes_per_pixel;
		size_t x_offset = static_cast<size_t>(x_begin) * buffer.bytes_per_pixel;

		u8* row = (u8*)buffer.memory + y_begin * buffer_pitch + x_offset;

		for (u32 y = y_begin; y < y_end; ++y)
		{
			u32* pixel = (u32*)row;
			for (u32 x = x_begin; x < x_end; ++x)
			{
				*pixel++ = c;
			}

			row += buffer_pitch;
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
		size_t x_offset = static_cast<size_t>(x_begin) * buffer.bytes_per_pixel;

		u8* buffer_row = (u8*)buffer.memory + static_cast<size_t>(y_begin) * buffer_pitch + x_offset;
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


	static void draw_image(img::image_t const& image, PixelBuffer& buffer, img::pixel_range_t const& range)
	{
		assert(range.x_end > range.x_begin);
		assert(range.y_end > range.y_begin);

		u32 height = range.y_end - range.y_begin;
		u32 width = range.x_end - range.x_begin;

		if (image.height != height || image.width != width)
		{
			img::image_t resized_image;
			resized_image.height = height;
			resized_image.width = width;
			img::resize_image(image, resized_image);
			draw_image(resized_image, buffer, range.x_begin, range.y_begin);

			return;
		}

		draw_image(image, buffer, range.x_begin, range.y_begin);
	}

	
	static void load_next_image(AppState& state, PixelBuffer& buffer)
	{
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
			fill_buffer(buffer, img::to_pixel(0, 0, 255));
			return;
		}

		auto current_entry = state.image_files[state.current_index];

		img::image_t loaded_image;
		img::read_image_from_file(current_entry, loaded_image);

		draw_image(loaded_image, buffer, IMAGE_RANGE);
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


	static void initialize_memory(AppMemory& memory, AppState& state)
	{
		state.dir_started = false;
		state.dir_complete = false;

		state.image_files = dir::get_files_of_type(IMAGE_DIR, IMAGE_EXTENSION, MAX_IMAGES);
	}


	static void create_dir(fs::path const& dir)
	{
		if (fs::exists(dir))
			return;

		fs::create_directory(dir);
	}


	static void move_file(fs::path const& file, fs::path const& dst_dir)
	{
		if (!fs::is_regular_file(file) || !fs::is_directory(dst_dir))
		{
			return;
		}

		auto name = file.filename();
		auto dst = dst_dir / name;

		fs::rename(file, dst);
	}


	static void start_app(AppState& state)
	{
		state.app_started = true;

		create_dir(fs::path(PASS_DIR));
		create_dir(fs::path(FAIL_DIR));		
	}


	static void update_pass(AppState& state, PixelBuffer& buffer)
	{
		move_file(state.image_files[state.current_index], fs::path(PASS_DIR));
		draw_rect(buffer, PASS_RANGE, img::to_pixel(0, 255, 0));
	}


	static void update_fail(AppState& state, PixelBuffer& buffer)
	{
		move_file(state.image_files[state.current_index], fs::path(FAIL_DIR));
		draw_rect(buffer, FAIL_RANGE, img::to_pixel(255, 0, 0));
	}


	void update_and_render(AppMemory& memory, Input const& input, PixelBuffer& buffer)
	{
		assert(sizeof(AppState) <= memory.permanent_storage_size);

		auto& state = *(AppState*)memory.permanent_storage;
		if (!memory.is_initialized)
		{
			initialize_memory(memory, state);
			memory.is_initialized = true;
		}

		auto& keyboard = input.keyboard;
		auto& mouse = input.mouse;

		if (keyboard.space_bar.ended_down)
		{
			if (!state.app_started)
			{
				start_app(state);
			}

			draw_rect(buffer, PASS_RANGE, img::to_pixel(0, 255, 0));
			draw_rect(buffer, FAIL_RANGE, img::to_pixel(255, 0, 0));

			load_next_image(state, buffer);	
		}
		else if (state.app_started && mouse.left.ended_down)
		{
			u32 buffer_x = BUFFER_WIDTH * mouse.mouse_x;
			u32 buffer_y = BUFFER_HEIGHT * mouse.mouse_y;

			if (in_range(buffer_x, buffer_y, PASS_RANGE))
			{
				update_pass(state, buffer);
				load_next_image(state, buffer);
			}
			else if (in_range(buffer_x, buffer_y, FAIL_RANGE))
			{
				update_fail(state, buffer);
				load_next_image(state, buffer);
			}

			
		}
		
	}


	void end_program()
	{
		auto root = fs::path(IMAGE_DIR);
		auto pass = fs::path(PASS_DIR);
		auto fail = fs::path(FAIL_DIR);

		for (auto& entry : fs::directory_iterator(pass))
		{
			move_file(entry, root);
		}

		for (auto& entry : fs::directory_iterator(fail))
		{
			move_file(entry, root);
		}
	}
}