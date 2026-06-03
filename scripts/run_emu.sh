#!/usr/bin/env bash
# Run the zero-emu harness inside a bubblewrap fake-root so the library finds
# its hardcoded device paths (/root/res/..., /media/internal/...) without
# polluting the host or needing real root.
#
#   - /root/res            -> repo resources (atlas) + substitute fonts
#   - /media/internal      -> a writable repo-local state dir (settings.db)
#
# Substitute fonts are used as stand-ins for the device's pixel fonts (the real
# TTFs aren't in this repo); set ZERO_EMU_FONT=/path/to.ttf to override.
#
# Env passthrough: ZERO_EMU_SCALE, ZERO_EMU_HZ, ZERO_EMU_FULL_UI (see harness).
set -e

DIR="$(cd "$(dirname "$0")" && pwd -P)"
ROOT_DIR=$(realpath "$DIR/..")
BIN="$ROOT_DIR/build-emu/zero-emu"

if [ ! -x "$BIN" ]; then
  echo "zero-emu not built. Run ./scripts/build_emu.sh first." >&2
  exit 1
fi
if ! command -v bwrap >/dev/null 2>&1; then
  echo "bubblewrap (bwrap) is required for the fake-root. Install it." >&2
  exit 1
fi

# Real device fonts (not redistributable, so kept outside the repo). If present
# they're used as-is; otherwise we fall back to a substitute mono font so the
# UI still comes up. Override the dir with ZERO_EMU_FONTS_DIR.
FONTS_DIR="${ZERO_EMU_FONTS_DIR:-$HOME/.local/share/fonts}"

# Pick a substitute font (used only for any device font that's missing).
FONT="${ZERO_EMU_FONT:-}"
if [ -z "$FONT" ]; then
  for cand in \
    /usr/share/fonts/misc/ter-*.otb \
    /usr/share/fonts/noto/NotoSansMono-Regular.ttf \
    /usr/share/fonts/liberation/LiberationMono-Regular.ttf \
    /usr/share/fonts/TTF/DejaVuSansMono.ttf ; do
    [ -f "$cand" ] && { FONT="$cand"; break; }
  done
fi

# Stage the fake /root/res tree.
RES_SRC="$ROOT_DIR/res"
STAGE="$ROOT_DIR/build-emu/emu-root"
mkdir -p "$STAGE/res/fonts" "$ROOT_DIR/build-emu/media-internal/.system"
cp -f "$RES_SRC/zero_atlas-32bit.bmp" "$STAGE/res/zero_atlas-32bit.bmp"
for f in pixelated.ttf pixelsix14.ttf lo-res09-nar.ttf; do
  if [ -f "$FONTS_DIR/$f" ]; then
    cp -f "$FONTS_DIR/$f" "$STAGE/res/fonts/$f"
  elif [ -n "$FONT" ] && [ -f "$FONT" ]; then
    echo "zero-emu: $f not in $FONTS_DIR; using substitute $FONT" >&2
    cp -f "$FONT" "$STAGE/res/fonts/$f"
  else
    echo "Missing font $f and no substitute (set ZERO_EMU_FONTS_DIR or ZERO_EMU_FONT)" >&2
    exit 1
  fi
done
if [ -d "$FONTS_DIR" ]; then echo "zero-emu: device fonts from $FONTS_DIR"; fi

# Fresh tmpfs root so bwrap can create any mountpoint (incl. /media, which may
# not exist on the host), with just the essentials bound in for SDL + wayland.
exec bwrap \
  --tmpfs / \
  --ro-bind /usr /usr \
  --symlink usr/lib /lib \
  --symlink usr/lib /lib64 \
  --symlink usr/bin /bin \
  --symlink usr/bin /sbin \
  --ro-bind /etc /etc \
  --dev-bind /dev /dev \
  --proc /proc \
  --ro-bind /sys /sys \
  --bind /run /run \
  --tmpfs /tmp \
  --ro-bind-try "$HOME" "$HOME" \
  --bind "$ROOT_DIR" "$ROOT_DIR" \
  --tmpfs /root \
  --ro-bind "$STAGE/res" /root/res \
  --bind "$ROOT_DIR/build-emu/media-internal" /media/internal \
  -- "$BIN" "$@"
