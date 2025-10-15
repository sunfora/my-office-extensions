@echo off

set PROJECT_NAME=custom-ui
pushd build

if exist %PROJECT_NAME%.mox (
  del %PROJECT_NAME%.mox
)

robocopy ..\src plugin /mir > NUL
robocopy modules\dll plugin\bin /mir > NUL
mox create --source=plugin --package=%PROJECT_NAME%

popd
