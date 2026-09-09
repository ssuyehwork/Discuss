@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    set "TARGET=G:\C++\QuarkMeta\QuarkMeta\src"
) else (
    set "TARGET=%~1"
)

if not exist "%TARGET%" (
    echo 路径不存在: %TARGET%
    exit /b 1
)

if exist "%TARGET%\*" (
    rem 是目录，递归刷新
    for /r "%TARGET%" %%F in (*.cpp *.h *.hpp *.c *.cc) do (
        copy /b "%%F"+,, "%%F" >nul
    )
) else (
    rem 是单个文件
    copy /b "%TARGET%"+,, "%TARGET%" >nul
)

echo 完成。