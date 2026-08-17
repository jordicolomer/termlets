# Termlets

A desktop environment that runs on your terminal.

<p align="center">
  <img src="https://github.com/user-attachments/assets/b0f51105-e8cb-428a-8dc5-c5324b0a05db" width="900">
</p>

Termlets sits somewhere between a terminal multiplexer such as `tmux` or `screen` and a desktop environment such as KDE or Windows. Despite its desktop-like appearance, it runs entirely inside the terminal.

It supports common mouse interactions, including clicking, trackpad scrolling, and dragging windows. It also comes with everything built in, including a fully fledged file manager, text editor, and terminal emulator, all available out of the box.

It’s implemented entirely in C, with zero external dependencies, resulting in an exceptionally small binary footprint with just **180 KB**. For comparison, even one of the smallest and simplest text editors, **nano**, has a binary size of around **840 KB**.

Currently supported on macOS (with iTerm2), Linux, and Windows.

![Screenshot](https://github.com/user-attachments/assets/d024501a-0a2b-4a4f-ab94-b5b235528993)

## Features

- Multiple terminal windows
- File manager
- Built-in text editor
- Syntax highlighting
- Integrated terminal emulator
- Windows with tabs
- Keyboard-driven interface
- Lightweight and fast
- Written in C
- Zero dependencies
- Process manager (coming soon)
- Multiple desktops (coming soon)
- Chess (coming soon)


## Installation

### macOS

Download the latest macOS binary:

```bash
curl -L https://github.com/jordicolomer/termlets/releases/download/v0.1.2/termlets-macos-arm64 -o termlets
```

Make it executable:

```bash
chmod +x termlets
```

Move it somewhere in your `PATH`:

```bash
sudo mv termlets /usr/local/bin/
```

You can now run it from anywhere:

```bash
termlets
```

### Apple Silicon

The current binary is built for macOS on Apple Silicon. If you're using an Intel Mac, you will need an Intel build.


### Linux

Download the latest Linux binary:

```bash
curl -L https://github.com/jordicolomer/termlets/releases/download/v0.1.2/termlets-linux-x86_64 -o termlets
```

Make it executable:

```bash
chmod +x termlets
```

Move it somewhere in your `PATH`:

```bash
sudo mv termlets /usr/local/bin/
```

You can now run it from anywhere:

```bash
termlets
```

### Windows

Download the latest Windows binary:

```powershell
curl -L https://github.com/jordicolomer/termlets/releases/download/v0.1.2/termlets-w64-x86_64.exe -o termlets.exe
```

Create a local bin directory and add it to your PATH (one-time setup):

```powershell
# Create the directory
New-Item -ItemType Directory -Force -Path "$env:USERPROFILE\.local\bin"

# Add to PATH for current session
$env:Path += ";$env:USERPROFILE\.local\bin"

# Add to PATH permanently (user-level, no admin required)
[Environment]::SetEnvironmentVariable("Path", [Environment]::GetEnvironmentVariable("Path", "User") + ";$env:USERPROFILE\.local\bin", "User")
```

Move the binary to the bin directory:

```powershell
Move-Item termlets.exe "$env:USERPROFILE\.local\bin\termlets.exe"
```

Restart your terminal, then you can run it from anywhere:

```powershell
termlets
```

**Requirements:**
- Windows 10 or later
- PowerShell 7 recommended for better UTF-8 support

## Default Keybindings

| Key          | Action          | Key     | Action          | Key     | Action          |
|--------------|-----------------|---------|-----------------|---------|-----------------|
| `Backspace`  | Backspace       | `n`     | Page up         | `w`     | Save            |
| `Enter`      | Enter           | `0`     | Start of line   | `r`     | Reload          |
| `;`          | Switch mode     | `$`     | End of line     | `Space` | Start selection |
| `h`          | Move left       | `g`     | First line      | `c`     | Copy            |
| `l`          | Move right      | `G`     | Last line       | `v`     | Paste           |
| `j`          | Move down       |         |                 | `x`     | Cut             |
| `k`          | Move up         |         |                 |         |                 |
| `p`          | Page down       |         |                 |         |                 |

## Building from Source

Clone the repository:

    git clone https://github.com/jordicolomer/termlets.git
    cd termlets

Build the project:

    make

Install the binary:

    sudo cp ./termlets /usr/local/bin/termlets

You can then run it from anywhere:

    termlets
