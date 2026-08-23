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


## Installation

### Install the latest Linux or macOS version

```bash
curl -fsSL https://raw.githubusercontent.com/jordicolomer/termlets/main/install.sh | sh
```

You can now run it from anywhere:

```bash
termlets
```

### Install the latest Windows version

Open cmd.exe or PowerShell and run:

```powershell
irm https://raw.githubusercontent.com/jordicolomer/termlets/main/install.ps1 | iex
```

The installer will download and install the latest version of termlets and add it to your user PATH.

Restart PowerShell so the updated PATH takes effect. You can then run termlets from any directory:

```powershell
termlets
```

**Requirements:**
- Windows 10 or later
- PowerShell 7 recommended for better UTF-8 support

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
