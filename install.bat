@echo off
setlocal

set "REPO=jordicolomer/termlets"
set "ASSET=termlets-w64-x86_64.exe"
set "INSTALL_DIR=%USERPROFILE%\.local\bin"
set "URL=https://github.com/%REPO%/releases/latest/download/%ASSET%"

echo Installing termlets...

if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

curl.exe -fL "%URL%" -o "%INSTALL_DIR%\termlets.exe"

if errorlevel 1 (
    echo Failed to download termlets.
    exit /b 1
)

powershell.exe -NoProfile -Command ^
    "$dir = [Environment]::ExpandEnvironmentVariables('%INSTALL_DIR%');" ^
    "$path = [Environment]::GetEnvironmentVariable('Path', 'User');" ^
    "if (($path -split ';') -notcontains $dir) {" ^
    "    [Environment]::SetEnvironmentVariable('Path', ($path.TrimEnd(';') + ';' + $dir), 'User')" ^
    "}"

echo.
echo termlets installed successfully!
echo Restart your terminal, then run:
echo.
echo     termlets

endlocal
