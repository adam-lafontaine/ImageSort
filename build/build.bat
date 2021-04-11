@echo off

rem cd /d D:\repos\ImageSort\build

set logfile=compile.log

set defines=/D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "NDEBUG"

set extra_ops=/permissive- /GS /GL /Gy /Zc:wchar_t /sdl /Zc:inline /fp:precise /errorReport:prompt /WX- /Zc:forScope /Gd /nologo /diagnostics:column
set opts=/Oi /EHa- /GR- /Gm- /nologo /FC /Zi /Fm /MT /EHsc %extra_ops%


set warnings=-W4 -wd4201 -wd4100 -wd4505 -wd4189
set standard=/std:c++17

rem "winmm.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib"

set win32_libs=user32.lib gdi32.lib winmm.lib kernel32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /MACHINE:X64 

set options=%defines% %opts% %warnings% %standard%

set dll_exports=/EXPORT:update_and_render /EXPORT:end_program

set dll_options=%options% /LD /link %dll_exports%

set root=D:\repos\ImageSort\

set utils=%root%\utils\

set utils_cpp=%utils%\dirhelper.cpp %utils%\libimage\libimage.cpp

set win_main=%root%\Win32UserSelect\src\Win32UserSelect.cpp
set win_main_cpp=%win_main% %utils_cpp%

set app_cpp=%root%\application\app.cpp
set dll_cpp=%app_cpp% %utils_cpp%

echo %time% > %logfile%

cl %dll_cpp% %dll_options% >> %logfile%
cl %win_main_cpp% %options% %win32_libs% >> %logfile%

echo %time% >> %logfile%