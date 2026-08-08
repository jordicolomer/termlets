# Termlets

A lightweight terminal multiplexer and desktop environment written in C.

# Screenshot
![Screenshot](media/Screenshot1.png)

# video
![Demo](media/demo.gif)

Termlet sits somewhere between a terminal multiplexer such as `tmux` or `screen` and a desktop environment such as KDE or Windows. Despite its desktop-like appearance, it runs entirely inside the terminal.

It supports common mouse interactions, including clicking, trackpad scrolling, and dragging windows. It also comes with everything built in, including a fully fledged file manager, text editor, and terminal emulator, all available out of the box.


It’s implemented entirely in C, with zero external dependencies, resulting in an exceptionally small binary footprint with just **180 KB**. For comparison, even one of the smallest and simplest text editors, **nano**, has a binary size of around **840 KB**.

It has currently only been tested on macOS using the iTerm2 terminal emulator. Support for the standard macOS Terminal is planned, along with Linux and Windows builds.


## Features

- Multiple terminal windows
- Built-in text editor
- Syntax highlighting
- File manager
- Keyboard-driven interface
- Lightweight and fast
- Written in C
- Zero dependencies
- total binary footprint only 180K
- Process manager (coming soon)
- Multiple desktops (coming soon)
- Chess (coming soon)


## Installation

```bash
git clone https://github.com/jordicolomer/termlets.git
cd termlets
make
sudo cp ./termlets /usr/local/bin/termlet
