// Win32UserSelect.cpp : Defines the entry point for the application.
//
#include "Win32UserSelect.h"

#include <iostream>

// app buffer will be scaled to these dimensions
constexpr int WINDOW_AREA_HEIGHT = 500;
constexpr int WINDOW_AREA_WIDTH = WINDOW_AREA_HEIGHT * 16 / 9;

constexpr r32 DEFAULT_MONITOR_REFRESH_HZ = 60.0f;
constexpr r32 TARGET_SECONDS_PER_FRAME = 1.0f / (DEFAULT_MONITOR_REFRESH_HZ / 2.0f);




namespace dll
{
    typedef struct app_code_t
    {
        LPCSTR dll_filename = app::DLL_FILENAME;
        LPCSTR dll_copy = app::DLL_COPY_FILENAME;

        HMODULE app_code_dll;
        FILETIME dll_last_write_time;

        bool is_initialized = false;
        bool is_valid = false;

        app::update_and_render_f update_and_render;
        app::end_program_f end_program;

    } AppCode;


    static FILETIME get_last_dll_write_time(LPCSTR filename)
    {
        FILETIME last_write_time = {};
        WIN32_FILE_ATTRIBUTE_DATA data = {};
        if (GetFileAttributesExA(filename, GetFileExInfoStandard, &data))
        {
            last_write_time = data.ftLastWriteTime;
        }

        return last_write_time;
    }


    static void update_and_render_stub(app::AppMemory&, app::Input const&, app::PixelBuffer&) {}


    static void end_program_stub() {}


    static void unload_app_code(AppCode& app_code)
    {
        if (app_code.app_code_dll)
        {
            FreeLibrary(app_code.app_code_dll);
            app_code.app_code_dll = 0;
        }

        app_code.update_and_render = update_and_render_stub;
        app_code.end_program = end_program_stub;
    }


    static void load_app_code(AppCode& app_code)
    {
        app_code.dll_last_write_time = get_last_dll_write_time(app_code.dll_filename);

        CopyFileA(app_code.dll_filename, app_code.dll_copy, FALSE);

        auto error = GetLastError();

        app_code.app_code_dll = LoadLibraryA(app_code.dll_copy);

        error = GetLastError();

        app_code.update_and_render = update_and_render_stub;
        app_code.end_program = end_program_stub;
        app_code.is_valid = false;

        if (!app_code.app_code_dll)
        {
            OutputDebugStringA("Could not load game code\n");
            return;
        }

        auto ur_id = GetProcAddress(app_code.app_code_dll, "update_and_render");
        auto ep_id = GetProcAddress(app_code.app_code_dll, "end_program");

        if(ur_id && ep_id)
        {
            app_code.update_and_render = win32::to_function<app::update_and_render_params>(ur_id);
            app_code.end_program = win32::to_function<app::end_program_params>(ep_id);
            app_code.is_valid = true;
        }
    }


}


GlobalVariable b32 g_running = false;
GlobalVariable win32::BitmapBuffer g_back_buffer = {};
GlobalVariable i64 g_perf_count_frequency;
GlobalVariable WINDOWPLACEMENT g_window_placement = { sizeof(g_window_placement) };
GlobalVariable dll::AppCode g_app_code = {};


void update_app_code()
{
#ifdef DLL_NO_HOTLOAD

    if (!g_app_code.is_initialized)
    {
        g_app_code.update_and_render = app::update_and_render;
        g_app_code.end_program = app::end_program;
        g_app_code.is_initialized = true;
    }

#else

    if (!g_app_code.is_initialized)
    {
        dll::load_app_code(g_app_code);
        g_app_code.is_initialized = true;
        return;
    }

    FILETIME writetime = dll::get_last_dll_write_time(g_app_code.dll_filename);
    if(CompareFileTime(&writetime, &g_app_code.dll_last_write_time) != 0)
    {
        dll::unload_app_code(g_app_code);
        dll::load_app_code(g_app_code);
    }


#endif // DLL_NO_HOTLOAD
}


void end_program()
{
    g_running = false;
    g_app_code.end_program();
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


    static LARGE_INTEGER get_wall_clock()
    {
        LARGE_INTEGER result;
        QueryPerformanceCounter(&result);

        return result;
    }


    static r32 get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
    {
        return (r32)(end.QuadPart - start.QuadPart) / g_perf_count_frequency;
    }


    static void record_input_message(app::ButtonState const& old_state, app::ButtonState& new_state, b32 is_down)
    {
        new_state.pressed = !old_state.ended_down && is_down;

        new_state.ended_down = is_down;        
    }


    static void record_mouse_input(HWND window, app::MouseInput& old_input, app::MouseInput& new_input)
    {
        app::reset_mouse(new_input);
        
        POINT mouse_pos;
        GetCursorPos(&mouse_pos);
        ScreenToClient(window, &mouse_pos);

        new_input.mouse_x = static_cast<r32>(mouse_pos.x) / WINDOW_AREA_WIDTH;
        new_input.mouse_y = static_cast<r32>(mouse_pos.y) / WINDOW_AREA_HEIGHT;
        new_input.mouse_z = 0.0f;

        auto const button_is_down = [](int btn) { return GetKeyState(btn) & (1u << 15); };

        record_input_message(old_input.left, new_input.left, button_is_down(VK_LBUTTON));
        record_input_message(old_input.middle, new_input.middle, button_is_down(VK_MBUTTON));
        record_input_message(old_input.right, new_input.right, button_is_down(VK_RBUTTON));
        
        // VK_XBUTTON1, VK_XBUTTON2
    }


    static void record_keyboard_input(app::KeyboardInput& old_input, app::KeyboardInput& new_input)
    {
        app::reset_keyboard(new_input);

        auto const key_was_down = [](MSG const& msg) { return (msg.lParam & (1u << 30)) != 0; };
        auto const key_is_down = [](MSG const& msg) { return (msg.lParam & (1u << 31)) == 0; };
        auto const alt_key_down = [](MSG const& msg) { return (msg.lParam & (1u << 29)); };

        MSG message;

        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            switch (message.message)
            {
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP:
                {
                    bool was_down = key_was_down(message);
                    bool is_down = key_is_down(message);

                    if (was_down == is_down)
                        break;

                    switch (message.wParam)
                    {
                        case 'R':
                        {
                            record_input_message(old_input.r_key, new_input.r_key, is_down);
                        } break;

                        case 'G':
                        {
                            record_input_message(old_input.g_key, new_input.g_key, is_down);
                        } break;

                        case 'B':
                        {
                            record_input_message(old_input.b_key, new_input.b_key, is_down);
                        } break;

                        case VK_UP:
                        {
                            
                        } break;

                        case VK_LEFT:
                        {
                            
                        } break;

                        case VK_DOWN:
                        {
                            
                        } break;

                        case VK_RIGHT:
                        {
                            
                        } break;

                        case VK_ESCAPE:
                        {
                            
                        } break;

                        case VK_SPACE:
                        {
                            record_input_message(old_input.space_bar, new_input.space_bar, is_down);
                        } break;

                        case VK_RETURN :
                        {
                            if (is_down && alt_key_down(message))
                            {
                                toggle_fullscreen(message.hwnd);
                            }
                        } break;

                        case VK_F4:
                        {
                            if (is_down && alt_key_down(message))
                            {
                                end_program();
                            }
                        } break;
                    }


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

    std::cout<<"winmain\n";

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

    

    app::Input input[2] = {};
    auto new_input = &input[0];
    auto old_input = &input[1];

    std::cout<<"loop\n";
    g_running = true;
    while (g_running)
    {
        update_app_code();

        win32::record_keyboard_input(old_input->keyboard, new_input->keyboard);        
        win32::record_mouse_input(window, old_input->mouse, new_input->mouse);

        g_app_code.update_and_render(app_memory, *new_input, app_pixel_buffer);

        wait_for_framerate();

        win32::display_buffer_in_window(g_back_buffer, device_context);
        
        // swap inputs
        auto temp = new_input;
        new_input = old_input;
        old_input = temp;
    }

    std::cout<<"end\n";


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
