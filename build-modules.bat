@echo off
cl window\src\window.cpp /I lua\include  /LD /MT /O2 /Fo:window\build\ /link /out:window\build\window.dll /IMPLIB:window\build\window.lib /LIBPATH:lua\lib lua.lib user32.lib gdi32.lib
