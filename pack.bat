@echo off

rm build\test-plugin.mox
cp window\build\window.dll src\bin\window.dll
mox create --source=src --package=test-plugin
mv test-plugin.mox build\test-plugin.mox
