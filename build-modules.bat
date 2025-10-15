@echo off

if not exist build\modules (
  mkdir build\modules
)
if not exist build\modules\object-files (
  mkdir build\modules\object-files
)
if not exist build\modules\dll (
  mkdir build\modules\dll
)
if not exist build\modules\lib (
  mkdir build\modules\lib
)

cl modules\window\window.cpp /I lua\include  /LD /MT /O2 /Fo:build\modules\object-files\ /link /out:build\modules\dll\window.dll /IMPLIB:build\modules\lib\window.lib /LIBPATH:lua\lib lua.lib user32.lib gdi32.lib
