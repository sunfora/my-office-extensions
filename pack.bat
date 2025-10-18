@echo off

del build\test-plugin.mox
copy window\build\window.dll src\bin\window.dll
mox create --source=src --package=test-plugin
move test-plugin.mox build\test-plugin.mox
