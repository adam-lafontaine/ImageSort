#pragma once

#include <cstdint>
#include <functional>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using r32 = float;
using r64 = double;

using b32 = uint32_t;




namespace app
{
	typedef struct app_memory_t
	{
		b32 is_initialized;
		size_t permanent_storage_size;
		void* permanent_storage; // required to be zero at startup

		size_t transient_storage_size;
		void* transient_storage; // required to be zero at startup

	} AppMemory;


	using to_color32_func = std::function<u32(u8 red, u8 green, u8 blue)>;


	typedef struct pixel_buffer_t
	{
		void* memory;
		u32 width;
		u32 height;
		u32 bytes_per_pixel;

		to_color32_func to_color32;

	} PixelBuffer;


	typedef struct button_state_t
	{
		u32 half_transition_count;
		b32 ended_down;

	} ButtonState;


	typedef union keyboard_input_t
	{
		ButtonState keys[3];
		struct
		{
			ButtonState space_bar;
			ButtonState r_key;
			ButtonState g_key;
			ButtonState b_key;
		};

	} KeyboardInput;


	typedef struct mouse_input_t
	{
		r32 mouse_x;
		r32 mouse_y;
		r32 mouse_z;

		union
		{
			ButtonState buttons[3];
			struct
			{
				ButtonState left;
				ButtonState right;
				ButtonState middle;
			};
		};

	} MouseInput;


	typedef struct input_t
	{
		KeyboardInput keyboard;
		MouseInput mouse;

	} Input;


	
}


