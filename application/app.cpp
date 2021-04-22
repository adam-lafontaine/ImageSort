#include "app.hpp"
#include "../utils/libimage/libimage.hpp"
#include "../utils/dirhelper.hpp"

#include <algorithm>
#include <vector>

namespace img = libimage;
namespace dir = dirhelper;


typedef struct cat_info_t
{
	fs::path directory;
	img::pixel_t background_color;
	img::pixel_range_t buffer_range;
	img::hist_t hist;

} CategoryInfo;


using category_list_t = std::array<CategoryInfo, 3>;

img::pixel_range_t empty_range() { img::pixel_range_t r = {}; return r; }


typedef struct app_state_t
{
	bool app_started = false;

	dir::file_list_t image_files;
	u32 current_index;

	bool dir_started = false;
	bool dir_complete = false;

	img::pixel_range_t image_roi = empty_range();

	img::hist_t current_hist = img::empty_hist();

} AppState;


//======= CONFIG =======================

constexpr u32 MAX_IMAGES = 1000;

constexpr auto IMAGE_EXTENSION = ".png";
constexpr auto IMAGE_DIR = "D:/test_images/src_pass";


category_list_t categories = { {
	{ "D:/test_images/sorted_red",   img::to_pixel(255, 0, 0), empty_range(), img::empty_hist() },
	{ "D:/test_images/sorted_green", img::to_pixel(0, 255, 0), empty_range(), img::empty_hist() },
	{ "D:/test_images/sorted_blue",  img::to_pixel(0, 0, 255), empty_range(), img::empty_hist() }
} };


constexpr u32 SIDEBAR_XSTART = 0;
constexpr u32 SIDEBAR_XEND = app::BUFFER_WIDTH / 20;
constexpr u32 IMAGE_XSTART = SIDEBAR_XEND;
constexpr u32 IMAGE_XEND = app::BUFFER_WIDTH * 8 / 10;
constexpr u32 CATEGORY_XSTART = IMAGE_XEND;
constexpr u32 CATEGORY_XEND = app::BUFFER_WIDTH;

constexpr img::pixel_range_t SIDEBAR_RANGE  = { SIDEBAR_XSTART,  SIDEBAR_XEND,  0, app::BUFFER_HEIGHT };
constexpr img::pixel_range_t IMAGE_RANGE    = { IMAGE_XSTART,    IMAGE_XEND,    0, app::BUFFER_HEIGHT };
constexpr img::pixel_range_t CATEGORY_RANGE = { CATEGORY_XSTART, CATEGORY_XEND, 0, app::BUFFER_HEIGHT };


using PixelBuffer = app::pixel_buffer_t;
using AppMemory = app::AppMemory;



static u32 to_buffer_color(PixelBuffer const& buffer, img::pixel_t const& p)
{
	return buffer.to_color32(p.red, p.green, p.blue);
}


static img::view_t make_buffer_view(PixelBuffer const& buffer)
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


static b32 in_range(u32 x, u32 y, img::pixel_range_t const& range)
{
	return x >= range.x_begin && x < range.x_end&& y >= range.y_begin && y < range.y_end;
}


static void fill_buffer(PixelBuffer& buffer, img::pixel_t const& color)
{
	auto c = to_buffer_color(buffer, color);

	/*auto buffer_pitch = static_cast<size_t>(buffer.width) * buffer.bytes_per_pixel;

	u8* row = (u8*)buffer.memory;
	for (u32 y = 0; y < buffer.height; ++y)
	{
		u32* pixel = (u32*)row;
		for (u32 x = 0; x < buffer.width; ++x)
		{
			*pixel++ = c;
		}

		row += buffer_pitch;
	}*/
	auto size = static_cast<size_t>(buffer.width) * buffer.width;
	u32* pixel = (u32*)buffer.memory;
	for (size_t i = 0; i < size; ++i)
	{
		*pixel++ = c;
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

	auto c = to_buffer_color(buffer, color);

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

	for (u32 y = y_begin; y < y_end; ++y)
	{
		u32* buffer_pixel = (u32*)buffer_row;
		auto image_pixel = (img::pixel_t*)image_row;

		for (u32 x = x_begin; x < x_end; ++x)
		{
			*buffer_pixel = to_buffer_color(buffer, *image_pixel);

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


static void draw_relative_qty(u32 qty, u32 total, PixelBuffer& buffer, img::pixel_range_t const& range)
{
	img::pixel_t black = img::to_pixel(0, 0, 0);

	auto buffer_view = make_buffer_view(buffer);
	auto region = img::sub_view(buffer_view, range);

	u32 draw_zero = region.height - 1;
	u32 draw_max = 0;
	u32 max_pixels = draw_zero - draw_max;

	u32 qty_offset = total == 0 ? 0 : static_cast<u32>(max_pixels * (r32)qty / total);

	img::pixel_range_t bar_range = {};
	bar_range.x_begin = region.width / 3;
	bar_range.x_end = region.width * 2 / 3;
	bar_range.y_begin = draw_zero - qty_offset;
	bar_range.y_end = region.height;

	auto bar = img::sub_view(region, bar_range);

	std::fill(bar.begin(), bar.end(), black);
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
		return;
	}

	auto& current_entry = state.image_files[state.current_index];

	img::image_t loaded_image;
	img::read_image_from_file(current_entry, loaded_image);

	state.current_hist = img::calc_hist(img::sub_view(loaded_image, state.image_roi));

	draw_image(loaded_image, buffer, IMAGE_RANGE);
}


static void initialize_memory(AppMemory& memory, AppState& state)
{
	state.dir_started = false;
	state.dir_complete = false;

	state.image_files = dir::get_files_of_type(IMAGE_DIR, IMAGE_EXTENSION, MAX_IMAGES);

	state.image_roi = { 55, 445, 55, 445 }; // TODO: set by user
}


static void draw_stats(category_list_t const& categories, PixelBuffer& buffer)
{
	auto buffer_view = make_buffer_view(buffer);
	img::pixel_t color = img::to_pixel(50, 50, 50);

	for (auto const& cat : categories)
	{
		draw_rect(buffer, cat.buffer_range, cat.background_color);

		auto view = img::sub_view(buffer_view, cat.buffer_range);
		img::draw_histogram(cat.hist, view, color);
	}
}


static void append_histogram(img::hist_t const& src, img::hist_t& dst)
{
	for (size_t i = 0; i < src.size(); ++i)
	{
		dst[i] += src[i];
	}
}


static void start_app(AppState& state)
{
	state.app_started = true;

	u32 height = app::BUFFER_HEIGHT / static_cast<u32>(categories.size());
	u32 y_begin = 0;
	u32 y_end = height;

	for (auto& cat : categories)
	{
		cat.buffer_range = { CATEGORY_RANGE.x_begin, CATEGORY_RANGE.x_end, y_begin, y_end };
		y_begin += height;
		y_end += height;

		auto& dir = cat.directory;

		if (!fs::exists(dir))
		{
			state.app_started &= fs::create_directories(dir);
		}
		else
		{
			state.app_started &= fs::is_directory(dir);
		}
	}
}


static b32 start_or_skip_image_executed(AppState& state, Input const& input, PixelBuffer& buffer)
{
	auto condition_to_execute = !state.dir_complete && input.keyboard.space_key.pressed;

	if (!condition_to_execute)
		return false;

	if (!state.app_started)
	{
		start_app(state);
	}

	draw_stats(categories, buffer);
	load_next_image(state, buffer);

	return true;
}


static b32 move_image_executed(AppState& state, Input const& input, PixelBuffer& buffer)
{
	auto& mouse = input.mouse;

	auto condition_to_execute = !state.dir_complete && state.app_started && input.mouse.left.pressed;

	if (!condition_to_execute)
		return false;

	u32 buffer_x = static_cast<u32>(app::BUFFER_WIDTH * mouse.mouse_x);
	u32 buffer_y = static_cast<u32>(app::BUFFER_HEIGHT * mouse.mouse_y);

	if (!in_range(buffer_x, buffer_y, CATEGORY_RANGE))
		return true;

	for (auto& cat : categories)
	{
		if (in_range(buffer_x, buffer_y, cat.buffer_range))
		{
			append_histogram(state.current_hist, cat.hist);
			dir::move_file(state.image_files[state.current_index], cat.directory);

			draw_stats(categories, buffer);
			load_next_image(state, buffer);
		}
	}

	return true;
}


static b32 draw_blank_image_executed(AppState& state, Input const& input, PixelBuffer& buffer)
{
	auto condition_to_execute = state.dir_complete;

	if (!condition_to_execute)
		return false;

	draw_rect(buffer, IMAGE_RANGE, img::to_pixel(100, 100, 100));

	return true;
}



namespace app
{
	

	
	void update_and_render(AppMemory& memory, Input const& input, PixelBuffer& buffer)
	{
		assert(sizeof(AppState) <= memory.permanent_storage_size);

		auto& state = *(AppState*)memory.permanent_storage;
		if (!memory.is_app_initialized)
		{
			initialize_memory(memory, state);
			memory.is_app_initialized = true;
		}

		if (start_or_skip_image_executed(state, input, buffer))
		{
			return;
		}
		else if (move_image_executed(state, input, buffer))
		{
			return;
		}
		else if (draw_blank_image_executed(state, input, buffer))
		{
			return;
		}
		
	}


	void end_program()
	{	
		// move images back to their original directory for testing
		auto root = fs::path(IMAGE_DIR);

		for (auto const& cat : categories)
		{
			auto& dir = cat.directory;
			for (auto& entry : fs::directory_iterator(dir))
			{
				dir::move_file(entry, root);
			}
		}
	}
}
