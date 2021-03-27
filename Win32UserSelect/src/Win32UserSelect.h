#pragma once

#include "framework.h"
#include "resource.h"
#include "../../application/app_types.hpp"


namespace win32
{
    typedef struct memory_state_t
    {
        u64 total_size;
        void* memory_block;

    } MemoryState;


    typedef struct window_dims_t
    {
        int width;
        int height;

    } WindowDims;


    typedef struct bitmap_buffer_t
    {
        u8 bytes_per_pixel = 4;
        BITMAPINFO info;
        void* memory;
        int width;
        int height;
        //int pitch;

    } BitmapBuffer;
}

