/*

Copyright (c) 2021 Adam Lafontaine

*/

#include "libimage.hpp"
#include "stb_all.hpp"

#ifndef LIBIMAGE_NO_MATH
#include <numeric>
#endif // !LIBIMAGE_NO_MATH


namespace libimage
{

#ifndef LIBIMAGE_NO_COLOR

	void read_image_from_file(const char* img_path_src, image_t& image_dst)
	{
		int width = 0;
		int height = 0;
		int image_channels = 0;
		int desired_channels = 4;

		auto data = (rgba_pixel*)stbi_load(img_path_src, &width, &height, &image_channels, desired_channels);

		assert(data);
		assert(width);
		assert(height);

		image_dst.data = data;
		image_dst.width = width;
		image_dst.height = height;
	}


	void make_image(image_t& image_dst, u32 width, u32 height)
	{
		assert(width);
		assert(height);

		image_dst.width = width;
		image_dst.height = height;
		image_dst.data = (pixel_t*)malloc(sizeof(pixel_t) * width * height);

		assert(image_dst.data);
	}


	view_t make_view(image_t const& img)
	{
		assert(img.width);
		assert(img.height);
		assert(img.data);

		view_t view;

		view.image_data = img.data;
		view.image_width = img.width;
		view.x_begin = 0;
		view.y_begin = 0;
		view.x_end = img.width;
		view.y_end = img.height;
		view.width = img.width;
		view.height = img.height;

		return view;
	}


	view_t sub_view(image_t const& image, pixel_range_t const& range)
	{
		assert(image.width);
		assert(image.height);
		assert(image.data);

		view_t sub_view;

		sub_view.image_data = image.data;
		sub_view.image_width = image.width;
		sub_view.x_begin = range.x_begin;
		sub_view.y_begin = range.y_begin;
		sub_view.x_end = range.x_end;
		sub_view.y_end = range.y_end;
		sub_view.width = range.x_end - range.x_begin;
		sub_view.height = range.y_end - range.y_begin;

		assert(sub_view.width);
		assert(sub_view.height);

		return sub_view;
	}


	view_t sub_view(view_t const& view, pixel_range_t const& range)
	{
		assert(view.width);
		assert(view.height);
		assert(view.image_data);

		assert(range.x_begin >= view.x_begin);
		assert(range.x_end <= view.x_end);
		assert(range.y_begin >= view.y_begin);
		assert(range.y_end <= view.y_end);

		view_t sub_view;

		sub_view.image_data = view.image_data;
		sub_view.image_width = view.image_width;
		sub_view.x_begin = view.x_begin + range.x_begin;
		sub_view.y_begin = view.y_begin + range.y_begin;
		sub_view.x_end = view.x_begin + range.x_end;
		sub_view.y_end = view.y_begin + range.y_end;
		sub_view.width = range.x_end - range.x_begin;
		sub_view.height = range.y_end - range.y_begin;

		assert(sub_view.width);
		assert(sub_view.height);

		return sub_view;
	}


	view_t row_view(image_t const& image, u32 y)
	{
		pixel_range_t range;
		range.x_begin = 0;
		range.x_end = image.width;
		range.y_begin = y;
		range.y_end = y + 1;

		return sub_view(image, range);
	}


	view_t row_view(view_t const& view, u32 y)
	{
		pixel_range_t range;
		range.x_begin = 0;
		range.x_end = view.width;
		range.y_begin = y;
		range.y_end = y + 1;

		return sub_view(view, range);
	}


	view_t column_view(image_t const& image, u32 x)
	{
		pixel_range_t range;
		range.x_begin = x;
		range.x_end = x + 1;
		range.y_begin = 0;
		range.y_end = image.height;

		return sub_view(image, range);
	}


	view_t column_view(view_t const& view, u32 x)
	{
		pixel_range_t range;
		range.x_begin = x;
		range.x_end = x + 1;
		range.y_begin = 0;
		range.y_end = view.height;

		return sub_view(view, range);
	}


	view_t row_view(image_t const& image, u32 x_begin, u32 x_end, u32 y)
	{
		pixel_range_t range;
		range.x_begin = x_begin;
		range.x_end = x_end;
		range.y_begin = y;
		range.y_end = y + 1;

		return sub_view(image, range);
	}


	view_t row_view(view_t const& view, u32 x_begin, u32 x_end, u32 y)
	{
		pixel_range_t range;
		range.x_begin = x_begin;
		range.x_end = x_end;
		range.y_begin = y;
		range.y_end = y + 1;

		return sub_view(view, range);
	}


	view_t column_view(image_t const& image, u32 y_begin, u32 y_end, u32 x)
	{
		pixel_range_t range;
		range.x_begin = x;
		range.x_end = x + 1;
		range.y_begin = y_begin;
		range.y_end = y_end;

		return sub_view(image, range);
	}


	view_t column_view(view_t const& view, u32 y_begin, u32 y_end, u32 x)
	{
		pixel_range_t range;
		range.x_begin = x;
		range.x_end = x + 1;
		range.y_begin = y_begin;
		range.y_end = y_end;

		return sub_view(view, range);
	}



#ifndef LIBIMAGE_NO_WRITE

	void write_image(image_t const& image_src, const char* file_path_dst)
	{
		assert(image_src.width);
		assert(image_src.height);
		assert(image_src.data);

		int width = static_cast<int>(image_src.width);
		int height = static_cast<int>(image_src.height);
		int channels = static_cast<int>(RGBA_CHANNELS);
		auto const data = image_src.data;

		int result = 0;

		auto ext = fs::path(file_path_dst).extension();

		assert(ext == ".bmp" || ext == ".png");

		if (ext == ".bmp" || ext == ".BMP")
		{
			result = stbi_write_bmp(file_path_dst, width, height, channels, data);
		}
		else if (ext == ".png" || ext == ".PNG")
		{
			int stride_in_bytes = width * channels;

			result = stbi_write_png(file_path_dst, width, height, channels, data, stride_in_bytes);
		}
		else if (ext == ".jpg" || ext == ".jpeg" || ext == ".JPG" || ext == ".JPEG")
		{
			// TODO: quality?
			// stbi_write_jpg(char const *filename, int w, int h, int comp, const void *data, int quality);
		}

		assert(result);
	}


	static void make_image(view_t const& view, image_t& image_dst)
	{
		make_image(image_dst, view.width, view.height);

		std::transform(view.cbegin(), view.cend(), image_dst.begin(), [&](auto p) { return p; });
	}


	void write_view(view_t const& view_src, const char* file_path_dst)
	{
		image_t image;
		make_image(view_src, image);

		write_image(image, file_path_dst);
	}

#endif // !LIBIMAGE_NO_WRITE


#ifndef LIBIMAGE_NO_RESIZE

	void resize_image(image_t const& image_src, image_t& image_dst)
	{
		assert(image_src.width);
		assert(image_src.height);
		assert(image_src.data);
		assert(image_dst.width);
		assert(image_dst.height);

		int channels = static_cast<int>(RGBA_CHANNELS);

		int width_src = static_cast<int>(image_src.width);
		int height_src = static_cast<int>(image_src.height);
		int stride_bytes_src = width_src * channels;

		int width_dst = static_cast<int>(image_dst.width);
		int height_dst = static_cast<int>(image_dst.height);
		int stride_bytes_dst = width_dst * channels;

		int result = 0;

		image_dst.data = (pixel_t*)malloc(sizeof(pixel_t) * image_dst.width * image_dst.height);

		result = stbir_resize_uint8(
			(u8*)image_src.data, width_src, height_src, stride_bytes_src,
			(u8*)image_dst.data, width_dst, height_dst, stride_bytes_dst,
			channels);

		assert(result);
	}


	view_t make_resized_view(image_t const& img_src, image_t& img_dst)
	{
		resize_image(img_src, img_dst);

		return make_view(img_dst);
	}

#endif // !LIBIMAGE_NO_RESIZE

#endif // !LIBIMAGE_NO_COLOR
	
#ifndef LIBIMAGE_NO_GRAYSCALE
	void read_image_from_file(const char* file_path_src, gray::image_t& image_dst)
	{
		int width = 0;
		int height = 0;
		int image_channels = 0;
		int desired_channels = 1;

		auto data = (gray::pixel_t*)stbi_load(file_path_src, &width, &height, &image_channels, desired_channels);

		assert(data);
		assert(width);
		assert(height);

		image_dst.data = data;
		image_dst.width = width;
		image_dst.height = height;
	}


	void make_image(gray::image_t& image_dst, u32 width, u32 height)
	{
		assert(width);
		assert(height);

		image_dst.width = width;
		image_dst.height = height;
		image_dst.data = (gray::pixel_t*)malloc(sizeof(gray::pixel_t) * width * height);

		assert(image_dst.data);
	}


	gray::view_t make_view(gray::image_t const& img)
	{
		assert(img.width);
		assert(img.height);
		assert(img.data);

		gray::view_t view;

		view.image_data = img.data;
		view.image_width = img.width;
		view.x_begin = 0;
		view.y_begin = 0;
		view.x_end = img.width;
		view.y_end = img.height;
		view.width = img.width;
		view.height = img.height;

		return view;
	}


	gray::view_t sub_view(gray::image_t const& image, pixel_range_t const& range)
	{
		assert(image.width);
		assert(image.height);
		assert(image.data);

		gray::view_t sub_view;

		sub_view.image_data = image.data;
		sub_view.image_width = image.width;
		sub_view.x_begin = range.x_begin;
		sub_view.y_begin = range.y_begin;
		sub_view.x_end = range.x_end;
		sub_view.y_end = range.y_end;
		sub_view.width = range.x_end - range.x_begin;
		sub_view.height = range.y_end - range.y_begin;

		assert(sub_view.width);
		assert(sub_view.height);

		return sub_view;
	}


	gray::view_t sub_view(gray::view_t const& view, pixel_range_t const& range)
	{
		assert(view.width);
		assert(view.height);
		assert(view.image_data);

		assert(range.x_begin >= view.x_begin);
		assert(range.x_end <= view.x_end);
		assert(range.y_begin >= view.y_begin);
		assert(range.y_end <= view.y_end);

		gray::view_t sub_view;

		sub_view.image_data = view.image_data;
		sub_view.image_width = view.image_width;
		sub_view.x_begin = view.x_begin + range.x_begin;
		sub_view.y_begin = view.y_begin + range.y_begin;
		sub_view.x_end = view.x_begin + range.x_end;
		sub_view.y_end = view.y_begin + range.y_end;
		sub_view.width = range.x_end - range.x_begin;
		sub_view.height = range.y_end - range.y_begin;

		assert(sub_view.width);
		assert(sub_view.height);

		return sub_view;
	}


	gray::view_t row_view(gray::image_t const& image, u32 y)
	{
		pixel_range_t range;
		range.x_begin = 0;
		range.x_end = image.width;
		range.y_begin = y;
		range.y_end = y + 1;

		return sub_view(image, range);
	}


	gray::view_t row_view(gray::view_t const& view, u32 y)
	{
		pixel_range_t range;
		range.x_begin = 0;
		range.x_end = view.width;
		range.y_begin = y;
		range.y_end = y + 1;

		return sub_view(view, range);
	}


	gray::view_t column_view(gray::image_t const& image, u32 x)
	{
		pixel_range_t range;
		range.x_begin = x;
		range.x_end = x + 1;
		range.y_begin = 0;
		range.y_end = image.height;

		return sub_view(image, range);
	}


	gray::view_t column_view(gray::view_t const& view, u32 x)
	{
		pixel_range_t range;
		range.x_begin = x;
		range.x_end = x + 1;
		range.y_begin = 0;
		range.y_end = view.height;

		return sub_view(view, range);
	}


	gray::view_t row_view(gray::image_t const& image, u32 x_begin, u32 x_end, u32 y)
	{
		pixel_range_t range;
		range.x_begin = x_begin;
		range.x_end = x_end;
		range.y_begin = y;
		range.y_end = y + 1;

		return sub_view(image, range);
	}


	gray::view_t row_view(gray::view_t const& view, u32 x_begin, u32 x_end, u32 y)
	{
		pixel_range_t range;
		range.x_begin = x_begin;
		range.x_end = x_end;
		range.y_begin = y;
		range.y_end = y + 1;

		return sub_view(view, range);
	}


	gray::view_t column_view(gray::image_t const& image, u32 y_begin, u32 y_end, u32 x)
	{
		pixel_range_t range;
		range.x_begin = x;
		range.x_end = x + 1;
		range.y_begin = y_begin;
		range.y_end = y_end;

		return sub_view(image, range);
	}


	gray::view_t column_view(gray::view_t const& view, u32 y_begin, u32 y_end, u32 x)
	{
		pixel_range_t range;
		range.x_begin = x;
		range.x_end = x + 1;
		range.y_begin = y_begin;
		range.y_end = y_end;

		return sub_view(view, range);
	}


#ifndef LIBIMAGE_NO_WRITE

	void write_image(gray::image_t const& image_src, const char* file_path_dst)
	{
		assert(image_src.width);
		assert(image_src.height);
		assert(image_src.data);

		int width = static_cast<int>(image_src.width);
		int height = static_cast<int>(image_src.height);
		int channels = 1;
		auto const data = image_src.data;

		int result = 0;

		auto ext = fs::path(file_path_dst).extension();

		assert(ext == ".bmp" || ext == ".png");

		if (ext == ".bmp" || ext == ".BMP")
		{
			result = stbi_write_bmp(file_path_dst, width, height, channels, data);
		}
		else if (ext == ".png" || ext == ".PNG")
		{
			int stride_in_bytes = width * channels;

			result = stbi_write_png(file_path_dst, width, height, channels, data, stride_in_bytes);
		}
		else if (ext == ".jpg" || ext == ".jpeg" || ext == ".JPG" || ext == ".JPEG")
		{
			// TODO: quality?
			// stbi_write_jpg(char const *filename, int w, int h, int comp, const void *data, int quality);
		}

		assert(result);
	}


	static void make_image(gray::view_t const& view_src, gray::image_t& image_dst)
	{
		make_image(image_dst, view_src.width, view_src.height);

		std::transform(view_src.cbegin(), view_src.cend(), image_dst.begin(), [](auto p) { return p; });
	}


	void write_view(gray::view_t const& view_src, const char* file_path_dst)
	{
		gray::image_t image;
		make_image(view_src, image);

		write_image(image, file_path_dst);
	}

#endif // !LIBIMAGE_NO_WRITE


#ifndef LIBIMAGE_NO_RESIZE

	void resize_image(gray::image_t const& image_src, gray::image_t& image_dst)
	{
		assert(image_src.width);
		assert(image_src.height);
		assert(image_src.data);
		assert(image_dst.width);
		assert(image_dst.height);

		int channels = 1;

		int width_src = static_cast<int>(image_src.width);
		int height_src = static_cast<int>(image_src.height);
		int stride_bytes_src = width_src * channels;

		int width_dst = static_cast<int>(image_dst.width);
		int height_dst = static_cast<int>(image_dst.height);
		int stride_bytes_dst = width_dst * channels;

		int result = 0;

		image_dst.data = (gray::pixel_t*)malloc(sizeof(gray::pixel_t) * image_dst.width * image_dst.height);

		result = stbir_resize_uint8(
			(u8*)image_src.data, width_src, height_src, stride_bytes_src,
			(u8*)image_dst.data, width_dst, height_dst, stride_bytes_dst,
			channels);

		assert(result);
	}


	gray::view_t make_resized_view(gray::image_t const& image_src, gray::image_t& image_dst)
	{
		resize_image(image_src, image_dst);

		return make_view(image_dst);
	}

#endif // !LIBIMAGE_NO_RESIZE

#endif // !#ifndef LIBIMAGE_NO_GRAYSCALE

#ifndef LIBIMAGE_NO_MATH

#ifndef LIBIMAGE_NO_COLOR

	rgb_stats_t calc_stats(view_t const& view)
	{
		constexpr auto n_channels = RGBA_CHANNELS - 1;

		auto const divisor = CHANNEL_SIZE / N_HIST_BUCKETS;

		std::array<hist_t, n_channels> c_hists = { 0 };
		std::array<r32, n_channels> c_counts = { 0 };

		auto const update = [&](pixel_t const& p)
		{
			for (u32 c = 0; c < n_channels; ++c)
			{
				auto bucket = p.channels[c] / divisor;

				++c_hists[c][bucket];
				c_counts[c] += p.channels[c];
			}
		};

		std::for_each(view.cbegin(), view.cend(), update);

		auto num_pixels = static_cast<size_t>(view.width) * view.height;

		auto c_means = c_counts;
		for (u32 c = 0; c < n_channels; ++c)
		{
			c_means[c] /= num_pixels;
		}

		std::array<r32, n_channels> c_diff_sq_totals = { 0 };
		std::array<size_t, n_channels> c_qty_totals = { 0 };

		for (u32 bucket = 0; bucket < c_hists[0].size(); ++bucket)
		{
			for (u32 c = 0; c < n_channels; ++c)
			{
				auto qty = c_hists[c][bucket];

				if (!qty)
					continue;

				c_qty_totals[c] += qty;
				r32 diff = static_cast<r32>(bucket) - c_means[c];

				c_diff_sq_totals[c] += qty * diff * diff;
			}
		}

		rgb_stats_t rgb_stats;

		for (u32 c = 0; c < n_channels; ++c)
		{
			r32 std_dev = c_qty_totals[c] == 0 ? 0.0f : sqrtf(c_diff_sq_totals[c] / c_qty_totals[c]);
			rgb_stats.stats[c] = { c_means[c], std_dev, c_hists[c] };
		}

		return rgb_stats;
	}


	void draw_histogram(rgb_stats_t const& rgb_stats, image_t& image_dst)
	{
		assert(!image_dst.width);
		assert(!image_dst.height);
		assert(!image_dst.data);
		assert(N_HIST_BUCKETS < CHANNEL_SIZE);

		constexpr auto n_channels = RGBA_CHANNELS - 1;

		u32 const max_relative_qty = 200;
		u32 const channel_spacing = 1;
		u32 const channel_height = max_relative_qty + channel_spacing;
		u32 const image_height = channel_height * n_channels;

		u32 const n_buckets = static_cast<u32>(N_HIST_BUCKETS);

		u32 const bucket_width = 20;
		u32 const bucket_spacing = 1;
		u32 const image_width = n_buckets * (bucket_spacing + bucket_width) + bucket_spacing;

		pixel_t white = to_pixel(255, 255, 255);

		make_image(image_dst, image_width, image_height);
		std::fill(image_dst.begin(), image_dst.end(), white);

		for (u32 c = 0; c < n_channels; ++c)
		{
			auto& hist = rgb_stats.stats[c].hist;

			auto max = std::accumulate(hist.begin(), hist.end(), 0.0f);

			const auto norm = [&](u32 count)
			{
				return static_cast<u32>(count / max * max_relative_qty);
			};

			pixel_range_t bar_range;
			bar_range.x_begin = bucket_spacing;
			bar_range.x_end = bar_range.x_begin + bucket_width;
			bar_range.y_begin = 0;
			bar_range.y_end = channel_height * (c + 1);

			for (u32 bucket = 0; bucket < n_buckets; ++bucket)
			{
				bar_range.y_begin = bar_range.y_end - norm(hist[bucket]);
				if (bar_range.y_end > bar_range.y_begin)
				{
					u8 shade = 200; // n_buckets* (bucket + 1) - 1;
					pixel_t color = to_pixel(0, 0, 0, 255);
					color.channels[c] = shade;
					auto bar_view = sub_view(image_dst, bar_range);
					std::fill(bar_view.begin(), bar_view.end(), color);
				}

				bar_range.x_begin += (bucket_spacing + bucket_width);
				bar_range.x_end += (bucket_spacing + bucket_width);
			}
		}
	}
#endif // !LIBIMAGE_NO_COLOR

#ifndef LIBIMAGE_NO_GRAYSCALE
	stats_t calc_stats(gray::view_t const& view)
	{
		hist_t hist = { 0 };
		r32 count = 0.0f;

		auto const divisor = CHANNEL_SIZE / N_HIST_BUCKETS;

		auto const update = [&](gray::pixel_t const& shade)
		{
			auto bucket = shade / divisor;
			++hist[bucket];
			count += shade;
		};

		std::for_each(view.cbegin(), view.cend(), update);

		auto num_pixels = static_cast<size_t>(view.width) * view.height;

		auto mean = count / num_pixels;
		assert(mean >= 0);
		assert(mean < CHANNEL_SIZE);

		r32 diff_sq_total = 0.0f;
		size_t qty_total = 0;
		for (u32 bucket = 0; bucket < hist.size(); ++bucket)
		{
			auto qty = hist[bucket];

			if (!qty)
				continue;

			qty_total += qty;
			r32 diff = static_cast<r32>(bucket) - mean;

			diff_sq_total += qty * diff * diff;
		}

		r32 std_dev = qty_total == 0 ? 0.0f : sqrtf(diff_sq_total / qty_total);

		return { mean, std_dev, hist };
	}


	void draw_histogram(hist_t const& hist, gray::image_t& image_dst)
	{
		assert(!image_dst.width);
		assert(!image_dst.height);
		assert(!image_dst.data);
		assert(N_HIST_BUCKETS < CHANNEL_SIZE);
		assert(hist.size() == N_HIST_BUCKETS);

		u32 const max_relative_qty = 200;
		u32 const image_height = max_relative_qty + 1;

		u32 const n_buckets = static_cast<u32>(N_HIST_BUCKETS);

		u32 const bucket_width = 20;
		u32 const bucket_spacing = 1;
		u32 const image_width = n_buckets * (bucket_spacing + bucket_width) + bucket_spacing;

		make_image(image_dst, image_width, image_height);
		std::fill(image_dst.begin(), image_dst.end(), 255);

		auto max = std::accumulate(hist.begin(), hist.end(), 0.0f);

		const auto norm = [&](u32 count)
		{
			return max_relative_qty - static_cast<u32>(count / max * max_relative_qty);
		};

		pixel_range_t bar_range;
		bar_range.x_begin = bucket_spacing;
		bar_range.x_end = (bucket_spacing + bucket_width);
		bar_range.y_begin = 0;
		bar_range.y_end = image_height;

		for (u32 bucket = 0; bucket < n_buckets; ++bucket)
		{
			bar_range.y_begin = norm(hist[bucket]);

			if (bar_range.y_end > bar_range.y_begin)
			{
				u8 shade = 50;// n_buckets* (bucket + 1) - 1;
				auto bar_view = sub_view(image_dst, bar_range);
				std::fill(bar_view.begin(), bar_view.end(), shade);
			}

			bar_range.x_begin += (bucket_spacing + bucket_width);
			bar_range.x_end += (bucket_spacing + bucket_width);
		}

	}
#endif // !LIBIMAGE_NO_GRAYSCALE

#endif // !LIBIMAGE_NO_MATH

}

