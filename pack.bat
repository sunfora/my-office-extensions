@echo off

pushd build

if exist context.mox (
  del context.mox
)

mox create --source=..\src --package=context
popd
