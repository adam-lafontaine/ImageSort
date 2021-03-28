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
	typedef struct thread_context_t
	{
		int placeholder;

	} ThreadContext;


	typedef struct memory_state_t
	{
		b32 is_initialized;
		size_t permanent_storage_size;
		void* permanent_storage; // required to be zero at startup

		size_t transient_storage_size;
		void* transient_storage; // required to be zero at startup

	} MemoryState;


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
			ButtonState red;
			ButtonState green;
			ButtonState blue;
		};

	} KeyboardInput;


	typedef struct mouse_input_t
	{
		i32 mouse_x;
		i32 mouse_y;
		i32 mouse_z;

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


