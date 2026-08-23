#!/bin/sh

set -e

REPO="jordicolomer/termlets"

OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Darwin)
        case "$ARCH" in
            arm64)  ASSET="termlets-macos-arm64" ;;
            x86_64) ASSET="termlets-macos-x86_64" ;;
            *) echo "Unsupported macOS architecture: $ARCH"; exit 1 ;;
        esac
        ;;
    Linux)
        case "$ARCH" in
            x86_64)         ASSET="termlets-linux-x86_64" ;;
            aarch64|arm64)  ASSET="termlets-linux-arm64" ;;
            *) echo "Unsupported Linux architecture: $ARCH"; exit 1 ;;
        esac
        ;;
    *)
        echo "Unsupported operating system: $OS"
        exit 1
        ;;
esac

VERSION=$(curl -fsSL \
    "https://api.github.com/repos/$REPO/releases/latest" |
    grep '"tag_name":' |
    sed -E 's/.*"([^"]+)".*/\1/')

echo "Installing termlets $VERSION..."

curl -fL \
    "https://github.com/$REPO/releases/download/$VERSION/$ASSET" \
    -o termlets

chmod +x termlets
sudo mv termlets /usr/local/bin/termlets

echo "Installed termlets $VERSION"
