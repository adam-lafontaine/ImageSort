#pragma once
/*

Copyright (c) 2021 Adam Lafontaine

*/

//#define LIBIMAGE_NO_COLOR
//#define LIBIMAGE_NO_GRAYSCALE
//#define LIBIMAGE_NO_WRITE
//#define LIBIMAGE_NO_RESIZE
//#define LIBIMAGE_NO_FS
//#define LIBIMAGE_NO_ALGORITHM
//#define LIBIMAGE_NO_MATH

#include <cstdint>
#include <iterator>
#include <cassert>

#ifndef LIBIMAGE_NO_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif // !LIBIMAGE_NO_FS

#ifndef LIBIMAGE_NO_ALGORITHM
#include <functional>
#include <algorithm>
#include <execution>
#endif // !LIBIMAGE_NO_ALGORITHM

#ifndef LIBIMAGE_NO_MATH
#include <array>
#endif // !LIBIMAGE_NO_MATH

using u8 = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;
using r32 = float;

namespace libimage
{
	constexpr auto RGBA_CHANNELS = 4u;
	constexpr size_t CHANNEL_SIZE = 256; // 8 bit channel

#ifndef LIBIMAGE_NO_MATH

	constexpr size_t N_HIST_BUCKETS = 256; // use each shade for histograms

#endif // !LIBIMAGE_NO_MATH
	

	//======= image_view.hpp =============

	// region of interest in an image
	typedef struct
	{
		u32 x_begin;
		u32 x_end;   // one past last x
		u32 y_begin;
		u32 y_end;   // one past last y

	} pixel_range_t;

#ifndef LIBIMAGE_NO_COLOR

	// color pixel
	typedef union rgba_pixel_t
	{
		struct
		{
			u8 red;
			u8 green;
			u8 blue;
			u8 alpha;
		};

		u8 channels[RGBA_CHANNELS];

		u32 value;

	}rgba_pixel;

	using pixel_t = rgba_pixel;


	// color image
	// owns the memory
	class rgba_image_t
	{
	public:
		u32 width = 0;
		u32 height = 0;

		pixel_t* data = 0;

		~rgba_image_t()
		{
			if (data)
			{
				free(data);
			}
		}

		rgba_pixel_t* begin() { return data; }
		rgba_pixel_t* end() { return data + (u64)width * (u64)height; }
	};

	using image_t = rgba_image_t;


	// subset of existing image data
	class rgba_image_view_t
	{
	public:

		pixel_t* image_data = 0;
		u32 image_width = 0;

		u32 x_begin = 0;
		u32 x_end = 0;
		u32 y_begin = 0;
		u32 y_end = 0;

		u32 width = 0;
		u32 height = 0;

		pixel_t* row_begin(u32 y) const
		{
			assert(y < height);

			auto offset = (y_begin + y) * image_width + x_begin;

			auto ptr = image_data + static_cast<u64>(offset);
			assert(ptr);

			return ptr;
		}

		pixel_t* xy_at(u32 x, u32 y) const
		{
			assert(y < height);
			assert(x < width);
			return row_begin(y) + x;
		}


		/******* ITERATOR ************/

		class iterator
		{
		private:

			u32 loc_x = 0;
			u32 loc_y = 0;

			u32 x_begin = 0;
			u32 x_end = 0;
			u32 y_begin = 0;
			u32 y_end = 0;

			pixel_t* image_data = 0;
			u32 image_width = 0;

			pixel_t* loc_ptr() const
			{
				assert(loc_x >= x_begin);
				assert(loc_x < x_end);
				assert(loc_y >= y_begin);
				assert(loc_y < y_end);

				auto offset = loc_y * image_width + loc_x;
				auto ptr = image_data + static_cast<u64>(offset);
				assert(ptr);

				return ptr;
			}

			void next()
			{
				++loc_x;
				if (loc_x >= x_end)
				{
					loc_x = x_begin;
					++loc_y;
				}

				assert(loc_x >= x_begin);
				assert(loc_x <= x_end);
				assert(loc_y >= y_begin);
				assert(loc_y <= y_end);
			}

		public:

			using iterator_category = std::forward_iterator_tag;
			using value_type = pixel_t;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using reference = value_type&;

			explicit iterator() {}

			explicit iterator(rgba_image_view_t const& view)
			{
				image_data = view.image_data;
				image_width = view.image_width;

				x_begin = view.x_begin;
				x_end = view.x_end;
				y_begin = view.y_begin;
				y_end = view.y_end;

				loc_x = x_begin;
				loc_y = y_begin;
			}

			iterator end()
			{
				loc_x = x_end - 1;
				loc_y = y_end - 1;

				next();

				return *this;
			}

			iterator& operator ++ ()
			{
				next();

				return *this;
			}

			iterator operator ++ (int) { iterator result = *this; ++(*this); return result; }

			bool operator == (iterator other) const { return loc_x == other.loc_x && loc_y == other.loc_y; }

			bool operator != (iterator other) const { return !(*this == other); }

			reference operator * () const { return *loc_ptr(); }
		};


		/******* ITERATOR ************/

		iterator begin() { return iterator(*this); }

		iterator end() { return iterator(*this).end(); }

		iterator cbegin() const { return iterator(*this); }

		iterator cend() const { return iterator(*this).end(); }

	};

	using view_t = rgba_image_view_t;


	constexpr pixel_t to_pixel(u8 red, u8 green, u8 blue, u8 alpha)
	{
		pixel_t pixel{};
		pixel.red = red;
		pixel.green = green;
		pixel.blue = blue;
		pixel.alpha = alpha;

		return pixel;
	}


	constexpr pixel_t to_pixel(u8 red, u8 green, u8 blue)
	{
		return to_pixel(red, green, blue, 255);
	}


	constexpr pixel_t to_pixel(u8 value)
	{
		return to_pixel(value, value, value, 255);
	}

#endif // !LIBIMAGE_NO_COLOR

#ifndef LIBIMAGE_NO_GRAYSCALE
	namespace gray
	{
		// grayscale value as an unsigned 8-bit integer
		using pixel_t = u8;


		// grayscale image
		class image_t
		{
		public:
			u32 width = 0;
			u32 height = 0;

			pixel_t* data = 0;

			~image_t()
			{
				if (data)
				{
					free(data);
				}
			}

			pixel_t* begin() { return data; }
			pixel_t* end() { return data + (u64)width * (u64)height; }
		};


		// subset of grayscale image data
		class image_view_t
		{
		public:

			pixel_t* image_data = 0;
			u32 image_width = 0;

			u32 x_begin = 0;
			u32 x_end = 0;
			u32 y_begin = 0;
			u32 y_end = 0;

			u32 width = 0;
			u32 height = 0;

			pixel_t* row_begin(u32 y) const
			{
				auto offset = (y_begin + y) * image_width + x_begin;
				return image_data + (u64)offset;
			}

			pixel_t* xy_at(u32 x, u32 y) const
			{
				return row_begin(y) + x;
			}

			/******* ITERATOR ************/

			class iterator
			{
			private:

				u32 loc_x = 0;
				u32 loc_y = 0;

				u32 x_begin = 0;
				u32 x_end = 0;
				u32 y_begin = 0;
				u32 y_end = 0;

				pixel_t* image_data = 0;
				u32 image_width = 0;

				pixel_t* loc_ptr() const
				{
					assert(loc_x >= x_begin);
					assert(loc_x < x_end);
					assert(loc_y >= y_begin);
					assert(loc_y < y_end);

					auto offset = loc_y * image_width + loc_x;
					auto ptr = image_data + static_cast<u64>(offset);
					assert(ptr);

					return ptr;
				}

				void next()
				{
					++loc_x;
					if (loc_x >= x_end)
					{
						loc_x = x_begin;
						++loc_y;
					}

					assert(loc_x >= x_begin);
					assert(loc_x <= x_end);
					assert(loc_y >= y_begin);
					assert(loc_y <= y_end);
				}

			public:

				using iterator_category = std::forward_iterator_tag;
				using value_type = pixel_t;
				using difference_type = std::ptrdiff_t;
				using pointer = value_type*;
				using reference = value_type&;

				explicit iterator() {}

				explicit iterator(image_view_t const& view)
				{
					image_data = view.image_data;
					image_width = view.image_width;

					x_begin = view.x_begin;
					x_end = view.x_end;
					y_begin = view.y_begin;
					y_end = view.y_end;

					loc_x = x_begin;
					loc_y = y_begin;
				}

				iterator end()
				{
					loc_x = x_end - 1;
					loc_y = y_end - 1;
					next();

					return *this;
				}

				iterator& operator ++ ()
				{
					next();

					return *this;
				}

				iterator operator ++ (int) { iterator result = *this; ++(*this); return result; }

				bool operator == (iterator other) const { return loc_x == other.loc_x && loc_y == other.loc_y; }

				bool operator != (iterator other) const { return !(*this == other); }

				reference operator * () const { return *loc_ptr(); }
			};

			/******* ITERATOR ************/

			iterator begin() { return iterator(*this); }

			iterator end() { return iterator(*this).end(); }

			iterator cbegin() const { return iterator(*this); }

			iterator cend() const { return iterator(*this).end(); }
		};

		using view_t = image_view_t;


	}

	namespace grey = gray;

#endif // !LIBIMAGE_NO_GRAYSCALE

	//======= libimage.hpp ==================
#ifndef LIBIMAGE_NO_COLOR

	void read_image_from_file(const char* img_path_src, image_t& image_dst);

	void make_image(image_t& image_dst, u32 width, u32 height);

	view_t make_view(image_t const& image);

	view_t sub_view(image_t const& image, pixel_range_t const& range);

	view_t sub_view(view_t const& view, pixel_range_t const& range);

	view_t row_view(image_t const& image, u32 y);

	view_t row_view(view_t const& view, u32 y);

	view_t column_view(image_t const& image, u32 x);

	view_t column_view(view_t const& view, u32 x);

	view_t row_view(image_t const& image, u32 x_begin, u32 x_end, u32 y);

	view_t row_view(view_t const& view, u32 x_begin, u32 x_end, u32 y);

	view_t column_view(image_t const& image, u32 y_begin, u32 y_end, u32 x);

	view_t column_view(view_t const& view, u32 y_begin, u32 y_end, u32 x);

#ifndef LIBIMAGE_NO_WRITE

	void write_image(image_t const& image_src, const char* file_path_dst);

	void write_view(view_t const& view_src, const char* file_path_dst);

#endif // !LIBIMAGE_NO_WRITE


#ifndef LIBIMAGE_NO_RESIZE

	void resize_image(image_t const& image_src, image_t& image_dst);

	view_t make_resized_view(image_t const& image_src, image_t& image_dst);

#endif // !LIBIMAGE_NO_RESIZE

#endif // !LIBIMAGE_NO_COLOR


#ifndef LIBIMAGE_NO_GRAYSCALE
	void read_image_from_file(const char* file_path_src, gray::image_t& image_dst);

	void make_image(gray::image_t& image_dst, u32 width, u32 height);

	gray::view_t make_view(gray::image_t const& image);

	gray::view_t sub_view(gray::image_t const& image, pixel_range_t const& range);

	gray::view_t sub_view(gray::view_t const& view, pixel_range_t const& range);

	gray::view_t row_view(gray::image_t const& image, u32 y);

	gray::view_t row_view(gray::view_t const& view, u32 y);

	gray::view_t column_view(gray::image_t const& image, u32 y);

	gray::view_t column_view(gray::view_t const& view, u32 y);

	gray::view_t row_view(gray::image_t const& image, u32 x_begin, u32 x_end, u32 y);

	gray::view_t row_view(gray::view_t const& view, u32 x_begin, u32 x_end, u32 y);

	gray::view_t column_view(gray::image_t const& image, u32 y_begin, u32 y_end, u32 x);

	gray::view_t column_view(gray::view_t const& view, u32 y_begin, u32 y_end, u32 x);


#ifndef LIBIMAGE_NO_WRITE

	void write_image(gray::image_t const& image_src, const char* file_path_dst);

	void write_view(gray::view_t const& view_src, const char* file_path_dst);

#endif // !LIBIMAGE_NO_WRITE

#ifndef LIBIMAGE_NO_RESIZE

	void resize_image(gray::image_t const& img_src, gray::image_t& img_dst);

	gray::view_t make_resized_view(gray::image_t const& image_src, gray::image_t& image_dst);

#endif // !LIBIMAGE_NO_RESIZE

#endif // !LIBIMAGE_NO_GRAYSCALE

	//======= libimage_fs ===================
#ifndef LIBIMAGE_NO_FS

#ifndef LIBIMAGE_NO_COLOR

	inline void read_image_from_file(fs::path const& img_path_src, image_t& image_dst)
	{
		auto file_path_str = img_path_src.string();

		read_image_from_file(file_path_str.c_str(), image_dst);
	}


	inline void write_image(image_t const& image_src, fs::path const& file_path)
	{
		auto file_path_str = file_path.string();

		write_image(image_src, file_path_str.c_str());
	}


	inline void write_view(view_t const& view_src, fs::path const& file_path)
	{
		auto file_path_str = file_path.string();

		write_view(view_src, file_path_str.c_str());
	}
#endif // !LIBIMAGE_NO_COLOR

#ifndef LIBIMAGE_NO_GRAYSCALE
	inline void read_image_from_file(fs::path const& img_path_src, gray::image_t& image_dst)
	{
		auto file_path_str = img_path_src.string();

		return read_image_from_file(file_path_str.c_str(), image_dst);
	}


	inline void write_image(gray::image_t const& image_src, fs::path const& file_path_dst)
	{
		auto file_path_str = file_path_dst.string();

		write_image(image_src, file_path_str.c_str());
	}


	inline void write_view(gray::view_t const& view_src, fs::path const& file_path_dst)
	{
		auto file_path_str = file_path_dst.string();

		write_view(view_src, file_path_str.c_str());
	}
#endif // !LIBIMAGE_NO_GRAYSCALE

#endif // !LIBIMAGE_NO_FS

	//======= libimage_algorithm.hpp ===================
#ifndef LIBIMAGE_NO_ALGORITHM

#ifndef LIBIMAGE_NO_COLOR

	// for_each
	using fe_ref_t = std::function<void(pixel_t& p)>;
	using fe_cref_t = std::function<void(pixel_t const& p)>;

	// transform
	using tf_1src_func_t = std::function<pixel_t(pixel_t& p)>;
	using tf_2src_func_t = std::function<pixel_t(pixel_t& p1, pixel_t& p2)>;

	namespace seq
	{
		template<typename F>
		inline void for_each_pixel(image_t& image, F const& func)
		{
			std::for_each(image.begin(), image.end(), func);
		}


		template<typename F>
		inline void for_each_pixel(view_t& view, F const& func)
		{
			std::for_each(view.begin(), view.end(), func);
		}


		inline void transform_pixels(view_t& src, view_t& dst, tf_1src_func_t const& func)
		{
			assert(dst.width >= src.width);
			assert(dst.height >= src.height);

			std::transform(src.begin(), src.end(), dst.begin(), func);
		}


		inline void transform_pixels(view_t& src1, view_t& src2, view_t& dst, tf_2src_func_t const& func)
		{
			assert(src1.width == src2.width);
			assert(src1.height == src2.height);
			assert(dst.width >= src1.width);
			assert(dst.height >= src1.height);

			std::transform(src1.begin(), src1.end(), src2.begin(), dst.begin(), func);
		}
	}


	namespace par
	{
		template<typename F>
		inline void for_each_pixel(image_t& image, F const& func)
		{
			std::for_each(std::execution::par, image.begin(), image.end(), func);
		}


		template<typename F>
		inline void for_each_pixel(view_t& view, F const& func)
		{
			std::for_each(std::execution::par, view.begin(), view.end(), func);
		}


		inline void transform_pixels(view_t& src, view_t& dst, tf_1src_func_t const& func)
		{
			assert(dst.width >= src.width);
			assert(dst.height >= src.height);

			std::transform(std::execution::par, src.begin(), src.end(), dst.begin(), func);
		}


		inline void transform_pixels(view_t& src1, view_t& src2, view_t& dst, tf_2src_func_t const& func)
		{
			assert(src1.width == src2.width);
			assert(src1.height == src2.height);
			assert(dst.width >= src1.width);
			assert(dst.height >= src1.height);

			std::transform(std::execution::par, src1.begin(), src1.end(), src2.begin(), dst.begin(), func);
		}
	}
#endif // !LIBIMAGE_NO_COLOR

#ifndef LIBIMAGE_NO_GRAYSCALE
	namespace gray
	{
		// for_each
		using fe_ref_t = std::function<void(pixel_t& p)>;
		using fe_cref_t = std::function<void(pixel_t const& p)>;
		using fe_xy_t = std::function<void(u32 x, u32 y)>;


		using tf_1src_func_t = std::function<pixel_t(pixel_t& p)>;
		using tf_2src_func_t = std::function<pixel_t(pixel_t& p1, pixel_t& p2)>;
	}


	namespace seq
	{
		template<typename F>
		inline void for_each_pixel(gray::image_t& image, F const& func)
		{
			std::for_each(image.begin(), image.end(), func);
		}


		template<typename F>
		inline void for_each_pixel(gray::view_t& view, F const& func)
		{
			std::for_each(view.begin(), view.end(), func);
		}

		template<typename F>
		inline void for_each_pixel(gray::view_t const& view, F const& func)
		{
			std::for_each(view.cbegin(), view.cend(), func);
		}


		inline void transform_pixels(gray::view_t& src, gray::view_t& dst, gray::tf_1src_func_t const& func)
		{
			assert(dst.width >= src.width);
			assert(dst.height >= src.height);

			std::transform(src.begin(), src.end(), dst.begin(), func);
		}


		inline void transform_pixels(gray::view_t& src1, gray::view_t& src2, gray::view_t& dst, gray::tf_2src_func_t const& func)
		{
			assert(src1.width == src2.width);
			assert(src1.height == src2.height);
			assert(dst.width >= src1.width);
			assert(dst.height >= src1.height);

			std::transform(src1.begin(), src1.end(), src2.begin(), dst.begin(), func);
		}

	}


	namespace par
	{
		template<typename F>
		inline void for_each_pixel(gray::image_t& image, F const& func)
		{
			std::for_each(std::execution::par, image.begin(), image.end(), func);
		}


		template<typename F>
		inline void for_each_pixel(gray::view_t& view, F const& func)
		{
			std::for_each(std::execution::par, view.begin(), view.end(), func);
		}


		inline void transform_pixels(gray::view_t& src, gray::view_t& dst, gray::tf_1src_func_t const& func)
		{
			assert(dst.width >= src.width);
			assert(dst.height >= src.height);

			std::transform(std::execution::par, src.begin(), src.end(), dst.begin(), func);
		}


		inline void transform_pixels(gray::view_t& src1, gray::view_t& src2, gray::view_t& dst, gray::tf_2src_func_t const& func)
		{
			assert(src1.width == src2.width);
			assert(src1.height == src2.height);
			assert(dst.width >= src1.width);
			assert(dst.height >= src1.height);

			std::transform(std::execution::par, src1.begin(), src1.end(), src2.begin(), dst.begin(), func);
		}
	}
#endif // !LIBIMAGE_NO_GRAYSCALE

#endif // !LIBIMAGE_NO_ALGORITHM

	//======= libimage_math.hpp =========================
#ifndef LIBIMAGE_NO_MATH

	using hist_t = std::array<u32, N_HIST_BUCKETS>;


	typedef struct channel_stats_t
	{
		r32 mean;
		r32 std_dev;
		hist_t hist;

	} stats_t;


	typedef union rgb_channel_stats_t
	{
		struct
		{
			stats_t red;
			stats_t green;
			stats_t blue;
		};

		stats_t stats[3];

	} rgb_stats_t;


#ifndef LIBIMAGE_NO_COLOR

	rgb_stats_t calc_stats(view_t const& view);

	void draw_histogram(rgb_stats_t const& rgb_stats, image_t& image_dst);

#endif // !LIBIMAGE_NO_COLOR

#ifndef	LIBIMAGE_NO_GRAYSCALE
	stats_t calc_stats(gray::view_t const& view);

	void draw_histogram(hist_t const& hist, gray::image_t& image_dst);
#endif // !LIBIMAGE_NO_GRAYSCALE

#endif // !LIBIMAGE_NO_MATH


}
