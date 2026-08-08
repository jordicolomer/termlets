# Termlets

A lightweight terminal multiplexer and desktop environment written in C.

![Screenshot](media/Screenshot1.png)


Termlets sits somewhere between a terminal multiplexer such as `tmux` or `screen` and a desktop environment such as KDE or Windows. Despite its desktop-like appearance, it runs entirely inside the terminal.

It supports common mouse interactions, including clicking, trackpad scrolling, and dragging windows. It also comes with everything built in, including a fully fledged file manager, text editor, and terminal emulator, all available out of the box.


It’s implemented entirely in C, with zero external dependencies, resulting in an exceptionally small binary footprint with just **180 KB**. For comparison, even one of the smallest and simplest text editors, **nano**, has a binary size of around **840 KB**.

It has currently only been tested on macOS using the iTerm2 terminal emulator. Support for the standard macOS Terminal is planned, along with Linux and Windows builds.

<p align="center">
  <img src="https://github.com/user-attachments/assets/b0f51105-e8cb-428a-8dc5-c5324b0a05db" width="900">
</p>


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
curl -L https://github.com/jordicolomer/termlets/releases/download/v0.1.0/termlets -o termlets
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

## Build from source

```bash
git clone https://github.com/jordicolomer/termlets.git
cd termlets
make
sudo cp ./termlets /usr/local/bin/termlets
