#include "app.hpp"
#include "../utils/libimage/libimage.hpp"
#include "../utils/dirhelper.hpp"

#include <algorithm>
#include <execution>
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

using PixelRange = img::pixel_range_t;

PixelRange empty_range() { img::pixel_range_t r = {}; return r; }


enum class AppMode : u32
{
	None,
	Sort,
	SelectRegion
};


typedef struct app_state_t
{
	AppMode mode = AppMode::None;
	bool app_started = false;
	bool dir_started = false;
	bool dir_complete = false;

	dir::file_list_t image_files;
	u32 current_index;

	PixelRange image_roi = empty_range();

	img::hist_t current_hist = img::empty_hist();

} AppState;


typedef struct point_2d_u32_t
{
	u32 x;
	u32 y;

} P2u32;



//======= CONFIG =======================

constexpr u32 MAX_IMAGES = 1000;

constexpr auto IMAGE_EXTENSION = ".png";
constexpr auto IMAGE_DIR = "D:/test_images/src_pass";




category_list_t categories = { {
	{ "D:/test_images/sorted_red",   img::to_pixel(255, 0, 0), empty_range(), img::empty_hist() },
	{ "D:/test_images/sorted_green", img::to_pixel(0, 255, 0), empty_range(), img::empty_hist() },
	{ "D:/test_images/sorted_blue",  img::to_pixel(0, 0, 255), empty_range(), img::empty_hist() }
} };


constexpr u32 SIDEBAR_XSTART  = 0;
constexpr u32 SIDEBAR_XEND    = app::BUFFER_WIDTH  * 5 / 100;
constexpr u32 SIDEBAR_YSTART  = 0;
constexpr u32 ICON_HEIGHT     = SIDEBAR_XEND - SIDEBAR_XSTART;
constexpr u32 IMAGE_XSTART    = SIDEBAR_XEND;
constexpr u32 IMAGE_XEND      = app::BUFFER_WIDTH * 80 / 100;
constexpr u32 CATEGORY_XSTART = IMAGE_XEND;
constexpr u32 CATEGORY_XEND   = app::BUFFER_WIDTH;

constexpr PixelRange SIDEBAR_RANGE  = { SIDEBAR_XSTART,  SIDEBAR_XEND,  SIDEBAR_YSTART, app::BUFFER_HEIGHT };
constexpr PixelRange ICON_ROI_SELECT_RANGE = { SIDEBAR_XSTART, SIDEBAR_XEND, SIDEBAR_YSTART, ICON_HEIGHT };

constexpr PixelRange IMAGE_RANGE    = { IMAGE_XSTART,    IMAGE_XEND,    0, app::BUFFER_HEIGHT };
constexpr PixelRange CATEGORY_RANGE = { CATEGORY_XSTART, CATEGORY_XEND, 0, app::BUFFER_HEIGHT };

using PixelBuffer = app::pixel_buffer_t;
using AppMemory = app::AppMemory;


static u32 to_buffer_color(PixelBuffer const& buffer, img::pixel_t const& p)
{
	return buffer.to_color32(p.red, p.green, p.blue);
}


static img::pixel_t to_buffer_pixel(PixelBuffer const& buffer, img::pixel_t const& p)
{
	img::pixel_t bp = {};

	bp.value = to_buffer_color(buffer, p);

	return bp;
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


static P2u32 get_buffer_position(r32 mouse_x, r32 mouse_y)
{
	P2u32 pt =
	{
		static_cast<u32>(app::BUFFER_WIDTH * mouse_x),
		static_cast<u32>(app::BUFFER_HEIGHT * mouse_y)
	};

	return pt;
}


static b32 in_range(P2u32 const& pos, PixelRange const& range)
{
	return pos.x >= range.x_begin && pos.x < range.x_end && pos.y >= range.y_begin && pos.y < range.y_end;
}


static void append_histogram(img::hist_t const& src, img::hist_t& dst)
{
	for (size_t i = 0; i < src.size(); ++i)
	{
		dst[i] += src[i];
	}
}


static void fill_buffer(PixelBuffer const& buffer, img::pixel_t const& color)
{
	auto c = to_buffer_color(buffer, color);

	auto size = static_cast<size_t>(buffer.width) * buffer.width;
	u32* pixel = (u32*)buffer.memory;
	for (size_t i = 0; i < size; ++i)
	{
		*pixel++ = c;
	}
}


static void draw_rect(img::pixel_t const& color, PixelBuffer const& buffer, PixelRange const& range)
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

	PixelRange dst_range = { x_begin, x_end, y_begin, y_end };
	auto buffer_view = make_buffer_view(buffer);
	auto dst_view = img::sub_view(buffer_view, dst_range);

	auto bp = to_buffer_pixel(buffer, color);

	std::fill(std::execution::par, dst_view.begin(), dst_view.end(), bp);
}


static void draw_image(img::image_t const& image, PixelBuffer const& buffer, u32 x_begin, u32 y_begin)
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

	PixelRange dst_range = { x_begin, x_end, y_begin, y_end };
	auto buffer_view = make_buffer_view(buffer);
	auto dst_view = img::sub_view(buffer_view, dst_range);

	std::transform(std::execution::par, image.begin(), image.end(), dst_view.begin(), [&](img::pixel_t const& p) { return to_buffer_pixel(buffer, p); });
}


static void draw_image(img::image_t const& image, PixelBuffer const& buffer, PixelRange const& range)
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


static void draw_relative_qty(u32 qty, u32 total, PixelBuffer const& buffer, PixelRange const& range)
{
	img::pixel_t black = img::to_pixel(0, 0, 0);

	auto buffer_view = make_buffer_view(buffer);
	auto region = img::sub_view(buffer_view, range);

	u32 draw_zero = region.height - 1;
	u32 draw_max = 0;
	u32 max_pixels = draw_zero - draw_max;

	u32 qty_offset = total == 0 ? 0 : static_cast<u32>(max_pixels * (r32)qty / total);

	PixelRange bar_range = {};
	bar_range.x_begin = region.width / 3;
	bar_range.x_end = region.width * 2 / 3;
	bar_range.y_begin = draw_zero - qty_offset;
	bar_range.y_end = region.height;

	auto bar = img::sub_view(region, bar_range);

	std::fill(std::execution::par, bar.begin(), bar.end(), black);
}


static void load_next_image(AppState& state, PixelBuffer const& buffer)
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


static void draw_stats(category_list_t const& categories, PixelBuffer const& buffer)
{
	auto buffer_view = make_buffer_view(buffer);
	img::pixel_t color = to_buffer_pixel(buffer, img::to_pixel(50, 50, 50));

	for (auto const& cat : categories)
	{
		draw_rect(cat.background_color, buffer, cat.buffer_range);

		auto view = img::sub_view(buffer_view, cat.buffer_range);
		img::draw_histogram(cat.hist, view, color);
	}
}


static void draw_icon(PixelRange const& range, PixelBuffer const& buffer)
{
	auto background = img::to_pixel(150, 150, 150);
	auto line_color = img::to_pixel(0, 0, 0);
	u32 line_thickness = 2;

	u32 height = range.y_end - range.y_begin;
	u32 width = range.x_end - range.x_begin;

	u32 x_begin = range.x_begin + width * 20 / 100;
	u32 x_end = range.x_begin + width * 80 / 100;
	u32 y_begin = range.y_begin + height * 20 / 100;
	u32 y_end = range.y_begin + height * 80 / 100;

	draw_rect(background, buffer, range);

	PixelRange top = { x_begin, x_end, y_begin, y_begin + line_thickness };
	PixelRange bottom = { x_begin, x_end, y_end - line_thickness, y_end };
	PixelRange left = { x_begin, x_begin + line_thickness, y_begin + line_thickness, y_end - line_thickness };
	PixelRange right = { x_end - line_thickness, x_end, y_begin + line_thickness, y_end - line_thickness };

	draw_rect(line_color, buffer, top);
	draw_rect(line_color, buffer, bottom);
	draw_rect(line_color, buffer, left);
	draw_rect(line_color, buffer, right);
}


static void start_app(AppState& state, PixelBuffer const& buffer)
{
	state.app_started = true;
	state.mode = AppMode::Sort;

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

	u32 icon_x_begin = SIDEBAR_RANGE.x_begin;
	u32 icon_x_end = SIDEBAR_RANGE.x_end;
	u32 icon_y_begin = SIDEBAR_RANGE.y_begin;
	u32 icon_y_end = icon_y_begin + icon_x_end - icon_x_begin;

	draw_icon({ icon_x_begin, icon_x_end, icon_y_begin, icon_y_end }, buffer);
}


static b32 start_app_executed(Input const& input, AppState& state, PixelBuffer const& buffer)
{
	auto condition_to_execute = !state.app_started && input.keyboard.space_key.pressed;

	if (!condition_to_execute)
		return false;

	start_app(state, buffer);

	draw_stats(categories, buffer);
	load_next_image(state, buffer);

	return true;
}


static b32 skip_image_executed(Input const& input, AppState& state, PixelBuffer const& buffer)
{
  	auto condition_to_execute = state.app_started && !state.dir_complete  && input.keyboard.space_key.pressed;

	if (!condition_to_execute)
		return false;

	load_next_image(state, buffer);

	return true;
}


static b32 move_image_executed(Input const& input, AppState& state, PixelBuffer const& buffer)
{
	auto& mouse = input.mouse;
	auto buffer_pos = get_buffer_position(mouse.mouse_x, mouse.mouse_y);

	auto condition_to_execute = !state.dir_complete && input.mouse.left.pressed && in_range(buffer_pos, CATEGORY_RANGE);

	if (!condition_to_execute)
		return false;

	for (auto& cat : categories)
	{
		if (in_range(buffer_pos, cat.buffer_range))
		{
			append_histogram(state.current_hist, cat.hist);
			dir::move_file(state.image_files[state.current_index], cat.directory);

			draw_stats(categories, buffer);
			load_next_image(state, buffer);

			break;
		}
	}

	return true;
}


static b32 draw_blank_image_executed(Input const& input, AppState& state, PixelBuffer const& buffer)
{
	auto condition_to_execute = state.dir_complete;

	if (!condition_to_execute)
		return false;

	draw_rect(img::to_pixel(100, 100, 100), buffer, IMAGE_RANGE);

	return true;
}


static b32 select_range_mode_executed(Input const& input, AppState& state, PixelBuffer const& buffer)
{
	auto& mouse = input.mouse;
	auto buffer_pos = get_buffer_position(mouse.mouse_x, mouse.mouse_y);

	auto condition_to_execute = !state.dir_complete && input.mouse.left.pressed && in_range(buffer_pos, ICON_ROI_SELECT_RANGE);

	if (!condition_to_execute)
		return false;

	state.mode = state.mode == AppMode::SelectRegion ? AppMode::Sort : AppMode::SelectRegion;

	return true;
}



namespace app
{
	

	
	void update_and_render(AppMemory& memory, Input const& input, PixelBuffer const& buffer)
	{
		assert(sizeof(AppState) <= memory.permanent_storage_size);

		auto& state = *(AppState*)memory.permanent_storage;
		if (!memory.is_app_initialized)
		{
			initialize_memory(memory, state);
			memory.is_app_initialized = true;
		}

		switch (state.mode)
		{
		case AppMode::None:

			if (start_app_executed(input, state, buffer))
			{
				return;
			}			

			break;

		case AppMode::Sort:

			if (select_range_mode_executed(input, state, buffer))
			{
				return;
			}
			
			if (move_image_executed(input, state, buffer))
			{
				return;
			}

			else if (skip_image_executed(input, state, buffer))
			{
				return;
			}
			
			if (draw_blank_image_executed(input, state, buffer))
			{
				return;
			}

			break;

		case AppMode::SelectRegion:

			if (select_range_mode_executed(input, state, buffer))
			{
				return;
			}

			break;
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
