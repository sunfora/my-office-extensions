@echo off

pushd build

if exist rmdup.mox (
  del rmdup.mox
)

mox create --source=..\src --package=rmdup
popd
