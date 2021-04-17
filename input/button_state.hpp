#pragma once

#include "../utils/typedefs.hpp"

typedef union button_state_t
{
	b32 states[2];
	struct
	{
		b32 pressed;
		b32 is_down;
	};


} ButtonState;
