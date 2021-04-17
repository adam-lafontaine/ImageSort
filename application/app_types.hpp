#pragma once

#include "../utils/typedefs.hpp"

#include <functional>


namespace app
{
	//typedef struct app_memory_t
	//{
	//	b32 is_app_initialized;
	//	size_t permanent_storage_size;
	//	void* permanent_storage; // required to be zero at startup

	//	size_t transient_storage_size;
	//	void* transient_storage; // required to be zero at startup

	//} AppMemory;


	//using to_color32_f = std::function<u32(u8 red, u8 green, u8 blue)>;


	//typedef struct pixel_buffer_t
	//{
	//	void* memory;
	//	u32 width;
	//	u32 height;
	//	u32 bytes_per_pixel;

	//	to_color32_f to_color32;

	//} PixelBuffer;


	/*typedef union button_state_t
	{
		b32 states[2];
		struct
		{
			b32 pressed;
			b32 is_down;
		};
		

	} ButtonState;*/


	/*inline void reset_button_state(ButtonState& state)
	{
		for (u32 i = 0; i < ArrayCount(state.states); ++i)
		{
			state.states[i] = false;
		}
	}*/


	/*typedef union keyboard_input_t
	{
		ButtonState keys[3];
		struct
		{
			ButtonState space_bar;
			ButtonState r_key;
			ButtonState g_key;
			ButtonState b_key;
		};

	} KeyboardInput;*/

	
	/*inline void reset_keyboard(KeyboardInput& keyboard)
	{
		for (u32 i = 0; i < ArrayCount(keyboard.keys); ++i)
		{
			reset_button_state(keyboard.keys[i]);
		}
	}*/


	/*typedef struct mouse_input_t
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

	} MouseInput;*/


	/*inline void reset_mouse(MouseInput& mouse)
	{
		mouse.mouse_x = 0.0f;
		mouse.mouse_y = 0.0f;
		mouse.mouse_z = 0.0f;

		for (u32 i = 0; i < ArrayCount(mouse.buttons); ++i)
		{
			reset_button_state(mouse.buttons[i]);
		}
	}*/


	/*typedef struct input_t
	{
		KeyboardInput keyboard;
		MouseInput mouse;

	} Input;*/


	
}


