#pragma once

#include <cstdint>

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

#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))
#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

#define GlobalVariable static
#define InternalFunction static


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


	typedef struct pixel_buffer_t
	{
		void* memory;
		u32 width;
		u32 height;
		u32 bytes_per_pixel;

	} PixelBuffer;


	typedef struct button_state_t
	{
		u32 half_transition_count;
		b32 ended_down;

	} ButtonState;


	typedef union keyboard_input_t
	{
		ButtonState keys[1];
		struct
		{
			ButtonState test;
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


