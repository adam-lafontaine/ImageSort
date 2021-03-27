// Win32UserSelect.cpp : Defines the entry point for the application.
//
#include "Win32UserSelect.h"

constexpr int SCREEN_BUFFER_WIDTH = 960;
constexpr int SCREEN_BUFFER_HEIGHT = 540;

constexpr r32 DEFAULT_MONITOR_REFRESH_HZ = 60.0f;
constexpr r32 TARGET_SECONDS_PER_FRAME = 1.0f / (DEFAULT_MONITOR_REFRESH_HZ / 2.0f);


GlobalVariable b32 g_running = false;
GlobalVariable win32::BitmapBuffer g_back_buffer = {};
GlobalVariable i64 g_perf_count_frequency;

namespace win32
{
    InternalFunction void resize_offscreen_buffer(BitmapBuffer& buffer, int width, int height)
    {
        if (buffer.memory)
        {
            VirtualFree(buffer.memory, 0, MEM_RELEASE);
        }

        buffer.width = width;
        buffer.height = height;

        buffer.info.bmiHeader.biSize = sizeof(buffer.info.bmiHeader);
        buffer.info.bmiHeader.biWidth = width;
        buffer.info.bmiHeader.biHeight = -height; // top down
        buffer.info.bmiHeader.biPlanes = 1;
        buffer.info.bmiHeader.biBitCount = buffer.bytes_per_pixel * 8; // 3 bytes + 1 byte padding
        buffer.info.bmiHeader.biCompression = BI_RGB;

        int bitmap_memory_size = width * height * buffer.bytes_per_pixel;

        buffer.memory = (u8*)VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

    }


    InternalFunction void display_buffer_in_window(BitmapBuffer& buffer, HDC device_context)
    {
        StretchDIBits(
            device_context,
            0, 0, buffer.width, buffer.height, // dst
            0, 0, buffer.width, buffer.height, // src
            buffer.memory,
            &(buffer.info),
            DIB_RGB_COLORS, SRCCOPY
        );
    }


    InternalFunction WindowDims get_window_dimensions(HWND window)
    {
        RECT client_rect;
        GetClientRect(window, &client_rect);

        return {
            client_rect.right - client_rect.left,
            client_rect.bottom - client_rect.top
        };
    }


    InternalFunction LARGE_INTEGER get_wall_clock()
    {
        LARGE_INTEGER result;
        QueryPerformanceCounter(&result);

        return result;
    }


    InternalFunction r32 get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
    {
        return (r32)(end.QuadPart - start.QuadPart) / g_perf_count_frequency;
    }
}



InternalFunction app::MemoryState allocate_app_memory(win32::MemoryState& win32_memory)
{
    app::MemoryState memory = {};

    memory.permanent_storage_size = Megabytes(256);
    memory.transient_storage_size = Gigabytes(1);

    size_t total_size = memory.permanent_storage_size + memory.transient_storage_size;

    LPVOID base_address = 0;

    memory.permanent_storage = VirtualAlloc(base_address, (SIZE_T)total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    memory.transient_storage = (u8*)memory.permanent_storage + memory.permanent_storage_size;

    win32_memory.total_size = total_size;
    win32_memory.memory_block = memory.permanent_storage;

    return memory;
}


InternalFunction app::PixelBuffer make_app_pixel_buffer()
{
    app::PixelBuffer buffer = {};

    buffer.memory = g_back_buffer.memory;
    buffer.width = g_back_buffer.width;
    buffer.height = g_back_buffer.height;
    buffer.bytes_per_pixel = g_back_buffer.bytes_per_pixel;

    return buffer;
}




//======= WIN32 API ==================================================

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name





// Forward declarations of functions included in this code module:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


InternalFunction WNDCLASSEXW make_window_class(HINSTANCE hInstance)
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


InternalFunction HWND make_window(HINSTANCE hInstance)
{
    return CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
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

    // TODO: Place code here.

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

    win32::resize_offscreen_buffer(g_back_buffer, SCREEN_BUFFER_WIDTH, SCREEN_BUFFER_HEIGHT);
    if (!g_back_buffer.memory)
    {
        return 0;
    }

    app::ThreadContext thread = {};

    LARGE_INTEGER pf_result;
    QueryPerformanceFrequency(&pf_result);
    g_perf_count_frequency = pf_result.QuadPart;


    LARGE_INTEGER work_counter = win32::get_wall_clock();
    LARGE_INTEGER last_counter = work_counter;
    UINT desired_scheduler_ms = 1;
    b32 sleep_is_granular = timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR;
    auto const wait_for_framerate = [&]() 
    {
        work_counter = win32::get_wall_clock();
        r32 frame_seconds_elapsed = win32::get_seconds_elapsed(last_counter, work_counter);
        DWORD sleep_ms = (DWORD)(1000.0f * (TARGET_SECONDS_PER_FRAME - frame_seconds_elapsed));
        if (frame_seconds_elapsed < TARGET_SECONDS_PER_FRAME && sleep_is_granular && sleep_ms > 0)
        {
            Sleep(sleep_ms);

            while (frame_seconds_elapsed < TARGET_SECONDS_PER_FRAME)
            {
                frame_seconds_elapsed = win32::get_seconds_elapsed(last_counter, win32::get_wall_clock());
            }
        }
        else
        {
            // missed frame rate
        }

        last_counter = win32::get_wall_clock();
    };


    auto app_pixel_buffer = make_app_pixel_buffer();


    g_running = true;
    while (g_running)
    {
        // keyboard input
        // mouse input

        wait_for_framerate();

        win32::display_buffer_in_window(g_back_buffer, device_context);
        
        // swap inputs
    }





    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }


    ReleaseDC(window, device_context);

    return (int) msg.wParam;
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
            case IDM_EXIT:
                DestroyWindow(hWnd);
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
    case WM_DESTROY:
        PostQuitMessage(0);
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
