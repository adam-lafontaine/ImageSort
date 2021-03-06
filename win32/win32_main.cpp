// Win32UserSelect.cpp : Defines the entry point for the application.
//
#include "win32_main.h"
#include "../application/app.hpp"

#include <iostream>

// size of window
// bitmap buffer will be scaled to these dimensions Windows (StretchDIBits)
constexpr int WINDOW_AREA_HEIGHT = 600;
constexpr int WINDOW_AREA_WIDTH = WINDOW_AREA_HEIGHT * 16 / 9;

// control the framerate of the application
constexpr r32 TARGET_FRAMERATE_HZ = 60.0f;
constexpr r32 TARGET_SECONDS_PER_FRAME = 1.0f / TARGET_FRAMERATE_HZ;

// flag to signal when the application should terminate
GlobalVariable b32 g_running = false;

// contains the memory that the application will draw to
GlobalVariable win32::BitmapBuffer g_back_buffer = {};
GlobalVariable i64 g_perf_count_frequency;
GlobalVariable WINDOWPLACEMENT g_window_placement = { sizeof(g_window_placement) };


void end_program()
{
    g_running = false;
    app::end_program();
}


namespace win32
{
    static void resize_offscreen_buffer(BitmapBuffer& buffer, u32 width, u32 height)
    {
        if (buffer.memory)
        {
            VirtualFree(buffer.memory, 0, MEM_RELEASE);
        }

        int iwidth = static_cast<int>(width);
        int iheight = static_cast<int>(height);

        buffer.width = iwidth;
        buffer.height = iheight;

        buffer.info.bmiHeader.biSize = sizeof(buffer.info.bmiHeader);
        buffer.info.bmiHeader.biWidth = iwidth;
        buffer.info.bmiHeader.biHeight = -iheight; // top down
        buffer.info.bmiHeader.biPlanes = 1;
        buffer.info.bmiHeader.biBitCount = buffer.bytes_per_pixel * 8; // 3 bytes + 1 byte padding
        buffer.info.bmiHeader.biCompression = BI_RGB;

        int bitmap_memory_size = iwidth * iheight * buffer.bytes_per_pixel;

        buffer.memory = (u8*)VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

    }


    static void display_buffer_in_window(BitmapBuffer& buffer, HDC device_context)
    {
        StretchDIBits(
            device_context,
            0, 0, WINDOW_AREA_WIDTH, WINDOW_AREA_HEIGHT, // dst
            0, 0, buffer.width, buffer.height, // src
            buffer.memory,
            &(buffer.info),
            DIB_RGB_COLORS, SRCCOPY
        );
    }


    static void toggle_fullscreen(HWND window)
    {
        // https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
        DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
        if (dwStyle & WS_OVERLAPPEDWINDOW)
        {
            MONITORINFO mi = { sizeof(mi) };
            if (GetWindowPlacement(window, &g_window_placement) && GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &mi))
            {
                SetWindowLong(window, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(window, HWND_TOP,
                    mi.rcMonitor.left, mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top,
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            }
        }
        else
        {
            SetWindowLong(window, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
            SetWindowPlacement(window, &g_window_placement);
            SetWindowPos(window, NULL, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }


    static WindowDims get_window_dimensions(HWND window)
    {
        RECT client_rect;
        GetClientRect(window, &client_rect);

        return {
            client_rect.right - client_rect.left,
            client_rect.bottom - client_rect.top
        };
    }


    static i64 get_perf_counter_frequency()
    {
        LARGE_INTEGER pf_result;
        QueryPerformanceFrequency(&pf_result);
        return pf_result.QuadPart;
    }
    
    
    static LARGE_INTEGER get_perf_counter()
    {
        LARGE_INTEGER result;
        QueryPerformanceCounter(&result);

        return result;
    }


    static r32 get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
    {
        return (r32)(end.QuadPart - start.QuadPart) / g_perf_count_frequency;
    }    


    static void process_keyboard_input(KeyboardInput const& old_input, KeyboardInput& new_input)
    {
        reset_keyboard(new_input);

        auto const key_was_down = [](MSG const& msg) { return (msg.lParam & (1u << 30)) != 0; };
        auto const key_is_down = [](MSG const& msg) { return (msg.lParam & (1u << 31)) == 0; };
        auto const alt_key_down = [](MSG const& msg) { return (msg.lParam & (1u << 29)); };

        MSG message;
        b32 was_down = false;
        b32 is_down = false;

        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            switch (message.message)
            {
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP:
                {
                    was_down = key_was_down(message);
                    is_down = key_is_down(message);

                    if (was_down == is_down)
                        break;

                    if (is_down && alt_key_down(message))
                    {
                        switch (message.wParam)
                        {
                        case VK_RETURN:
                            toggle_fullscreen(message.hwnd);
                            return;

                        case VK_F4:
                            end_program();
                            return;
                        }
                    }

                    record_keyboard_input(message.wParam, old_input, new_input, is_down);

                } break;

                default:
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
            }
        }
    }

}



static app::AppMemory allocate_app_memory(win32::MemoryState& win32_memory)
{
    app::AppMemory memory = {};

    memory.permanent_storage_size = Megabytes(256);
    memory.transient_storage_size = 0; // Gigabytes(1);

    size_t total_size = memory.permanent_storage_size + memory.transient_storage_size;

    LPVOID base_address = 0;

    memory.permanent_storage = VirtualAlloc(base_address, (SIZE_T)total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    memory.transient_storage = (u8*)memory.permanent_storage + memory.permanent_storage_size;

    win32_memory.total_size = total_size;
    win32_memory.memory_block = memory.permanent_storage;

    return memory;
}


static app::PixelBuffer make_app_pixel_buffer()
{
    app::PixelBuffer buffer = {};

    buffer.memory = g_back_buffer.memory;
    buffer.width = g_back_buffer.width;
    buffer.height = g_back_buffer.height;
    buffer.bytes_per_pixel = g_back_buffer.bytes_per_pixel;

    buffer.to_color32 = [](u8 red, u8 green, u8 blue) { return red << 16 | green << 8 | blue; };

    return buffer;
}


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


// Forward declarations of functions included in this code module:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


static WNDCLASSEXW make_window_class(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32USERSELECT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WIN32USERSELECT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return wcex;
}


static HWND make_window(HINSTANCE hInstance)
{
    int extra_width = 16;
    int extra_height = 59;

    return CreateWindowW( // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WINDOW_AREA_WIDTH + extra_width,
        WINDOW_AREA_HEIGHT + extra_height,
        nullptr,
        nullptr,
        hInstance,
        nullptr);
}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);


    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WIN32USERSELECT, szWindowClass, MAX_LOADSTRING);

    WNDCLASSEXW window_class = make_window_class(hInstance);
    if (!RegisterClassExW(&window_class))
    {
        return 0;
    }

    // Perform application initialization:
    hInst = hInstance; // Store instance handle in our global variable

    HWND window = make_window(hInstance);

    if (!window)
    {
        return 0;
    }

    ShowWindow(window, nCmdShow);
    UpdateWindow(window);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32USERSELECT));

    HDC device_context = GetDC(window);
    auto window_dims = win32::get_window_dimensions(window);

    win32::MemoryState win32_memory = {};
    auto app_memory = allocate_app_memory(win32_memory);
    if (!app_memory.permanent_storage || !app_memory.transient_storage)
    {
        return 0;
    }    

    win32::resize_offscreen_buffer(g_back_buffer, app::BUFFER_WIDTH, app::BUFFER_HEIGHT);
    if (!g_back_buffer.memory)
    {
        return 0;
    }

    auto app_pixel_buffer = make_app_pixel_buffer();

    // manage framerate
    g_perf_count_frequency = win32::get_perf_counter_frequency();
    LARGE_INTEGER work_counter = win32::get_perf_counter();
    LARGE_INTEGER last_counter = work_counter;
    UINT desired_scheduler_ms = 1;
    b32 sleep_is_granular = timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR;

    auto const wait_for_framerate = [&]() 
    {
        work_counter = win32::get_perf_counter();
        r32 frame_seconds_elapsed = win32::get_seconds_elapsed(last_counter, work_counter);
        DWORD sleep_ms = (DWORD)(1000.0f * (TARGET_SECONDS_PER_FRAME - frame_seconds_elapsed));
        if (frame_seconds_elapsed < TARGET_SECONDS_PER_FRAME && sleep_is_granular && sleep_ms > 0)
        {
            Sleep(sleep_ms);

            while (frame_seconds_elapsed < TARGET_SECONDS_PER_FRAME)
            {
                frame_seconds_elapsed = win32::get_seconds_elapsed(last_counter, win32::get_perf_counter());
            }
        }
        else
        {
            // missed frame rate
        }

        last_counter = win32::get_perf_counter();
    };
    

    Input input[2] = {};
    auto new_input = &input[0];
    auto old_input = &input[1];

    g_running = true;
    while (g_running)
    {
        win32::process_keyboard_input(old_input->keyboard, new_input->keyboard);        
        win32::record_mouse_input(window, old_input->mouse, new_input->mouse);
        app::update_and_render(app_memory, *new_input, app_pixel_buffer);

        wait_for_framerate();

        win32::display_buffer_in_window(g_back_buffer, device_context);
        
        // swap inputs
        auto temp = new_input;
        new_input = old_input;
        old_input = temp;
    }


    ReleaseDC(window, device_context);

    return 0;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;

            case IDM_EXIT: // File > Exit
                DestroyWindow(hWnd);
                end_program();
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            win32::display_buffer_in_window(g_back_buffer, hdc);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY: // X button
        PostQuitMessage(0);
        end_program();
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
