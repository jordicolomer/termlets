$ErrorActionPreference = "Stop"

$Repo = "jordicolomer/termlets"
$Asset = "termlets-windows-x86_64.exe"
$InstallDir = "$env:USERPROFILE\.local\bin"
$Url = "https://github.com/$Repo/releases/latest/download/$Asset"

Write-Host "Installing termlets..."

# Create installation directory
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

# Download latest release
$TempFile = Join-Path $env:TEMP "termlets.exe"

Invoke-WebRequest `
    -Uri $Url `
    -OutFile $TempFile

# Install
Move-Item -Force $TempFile "$InstallDir\termlets.exe"

# Add to PATH permanently if not already present
$UserPath = [Environment]::GetEnvironmentVariable("Path", "User")

if ($UserPath -notlike "*$InstallDir*") {
    [Environment]::SetEnvironmentVariable(
        "Path",
        "$UserPath;$InstallDir",
        "User"
    )
}

# Add to PATH for current PowerShell session
if ($env:Path -notlike "*$InstallDir*") {
    $env:Path += ";$InstallDir"
}

Write-Host "termlets installed successfully!"
Write-Host "Run: termlets"
