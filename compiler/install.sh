#!/usr/bin/env sh

set -eu

usage() {
    cat <<'EOF'
Usage: ./install.sh [--prefix DIR] [--bin-dir DIR] [--lib-dir DIR] [--help]

Builds Calynda and installs:
  - a launcher at <bin-dir>/calynda
  - the real CLI binary at <lib-dir>/calynda/calynda
  - the runtime object at <lib-dir>/calynda/runtime.o

Defaults:
  - root:    prefix=/usr/local
  - non-root: prefix=$HOME/.local

Overrides:
  PREFIX   Install prefix
  BIN_DIR  Install bin directory
  LIB_DIR  Install library directory

Examples:
  ./install.sh
  ./install.sh --prefix /usr/local
  BIN_DIR=$HOME/bin LIB_DIR=$HOME/.local/lib ./install.sh
EOF
}

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        printf 'error: required command not found: %s\n' "$1" >&2
        exit 1
    fi
}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)

PREFIX_SET=0
BIN_DIR_SET=0
LIB_DIR_SET=0

if [ "${PREFIX+x}" = x ]; then
    PREFIX_SET=1
else
    if [ "$(id -u)" -eq 0 ]; then
        PREFIX=/usr/local
    else
        PREFIX=$HOME/.local
    fi
fi

if [ "${BIN_DIR+x}" = x ]; then
    BIN_DIR_SET=1
else
    BIN_DIR=$PREFIX/bin
fi

if [ "${LIB_DIR+x}" = x ]; then
    LIB_DIR_SET=1
else
    LIB_DIR=$PREFIX/lib
fi

while [ "$#" -gt 0 ]; do
    case "$1" in
        --prefix)
            [ "$#" -ge 2 ] || {
                printf 'error: --prefix requires a value\n' >&2
                exit 1
            }
            PREFIX=$2
            PREFIX_SET=1
            if [ "$BIN_DIR_SET" -eq 0 ]; then
                BIN_DIR=$PREFIX/bin
            fi
            if [ "$LIB_DIR_SET" -eq 0 ]; then
                LIB_DIR=$PREFIX/lib
            fi
            shift 2
            ;;
        --bin-dir)
            [ "$#" -ge 2 ] || {
                printf 'error: --bin-dir requires a value\n' >&2
                exit 1
            }
            BIN_DIR=$2
            BIN_DIR_SET=1
            shift 2
            ;;
        --lib-dir)
            [ "$#" -ge 2 ] || {
                printf 'error: --lib-dir requires a value\n' >&2
                exit 1
            }
            LIB_DIR=$2
            LIB_DIR_SET=1
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            printf 'error: unknown argument: %s\n\n' "$1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

require_command make
require_command install
require_command gcc

BUILD_DIR=$SCRIPT_DIR/build
SOURCE_BIN=$BUILD_DIR/calynda
SOURCE_RUNTIME=$BUILD_DIR/runtime.o
INSTALL_ROOT=$LIB_DIR/calynda
INSTALL_BIN=$INSTALL_ROOT/calynda
INSTALL_RUNTIME=$INSTALL_ROOT/runtime.o
LAUNCHER_PATH=$BIN_DIR/calynda

printf '==> Building Calynda\n'
make -C "$SCRIPT_DIR" calynda >/dev/null

if [ ! -f "$SOURCE_BIN" ]; then
    printf 'error: build did not produce %s\n' "$SOURCE_BIN" >&2
    exit 1
fi

if [ ! -f "$SOURCE_RUNTIME" ]; then
    printf 'error: build did not produce %s\n' "$SOURCE_RUNTIME" >&2
    exit 1
fi

printf '==> Installing files\n'
install -d "$BIN_DIR" "$INSTALL_ROOT"
install -m 755 "$SOURCE_BIN" "$INSTALL_BIN"
install -m 644 "$SOURCE_RUNTIME" "$INSTALL_RUNTIME"

cat > "$LAUNCHER_PATH" <<EOF
#!/usr/bin/env sh
exec "$INSTALL_BIN" "\$@"
EOF
chmod 755 "$LAUNCHER_PATH"

printf '\nInstalled Calynda:\n'
printf '  launcher: %s\n' "$LAUNCHER_PATH"
printf '  binary:   %s\n' "$INSTALL_BIN"
printf '  runtime:  %s\n' "$INSTALL_RUNTIME"

case ":${PATH}:" in
    *":$BIN_DIR:"*)
        ;;
    *)
        printf '\n%s is not on your PATH. Add this line to your shell profile:\n' "$BIN_DIR"
        printf '  export PATH="%s:$PATH"\n' "$BIN_DIR"
        ;;
esac

printf '\nTry: calynda help\n'