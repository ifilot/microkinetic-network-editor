#!/usr/bin/env bash
set -euo pipefail

# ------------------------------------------------------------
# Configuration
# ------------------------------------------------------------
APP_NAME="mne"
BUILD_DIR="build-windows"
DIST_DIR="dist"

echo "==> Packaging ${APP_NAME} (Windows / MinGW / Qt5)"

# ------------------------------------------------------------
# Clean previous build
# ------------------------------------------------------------
rm -rf "${BUILD_DIR}" "${DIST_DIR}"
mkdir -p "${BUILD_DIR}" "${DIST_DIR}"

# ------------------------------------------------------------
# Configure
# ------------------------------------------------------------
echo "==> Configuring with CMake"
cmake -S . -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/mingw64

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------
echo "==> Building"
cmake --build "${BUILD_DIR}"

# ------------------------------------------------------------
# Copy executable
# ------------------------------------------------------------
echo "==> Copying executable"
cp "${BUILD_DIR}/${APP_NAME}.exe" "${DIST_DIR}/"

# ------------------------------------------------------------
# Copy examples directory (including subdirectories)
# ------------------------------------------------------------
echo "==> Copying examples directory"
cp -r examples "${DIST_DIR}/examples"

# Provide qmake.exe for windeployqt (MSYS2 requirement)
QMAKE_QT5="$MINGW_PREFIX/bin/qmake-qt5.exe"
QMAKE_SHIM="$MINGW_PREFIX/bin/qmake.exe"
if [[ ! -x "$QMAKE_SHIM" ]]; then
  echo "[INFO] Creating qmake.exe shim"
  ln -s qmake-qt5.exe "$QMAKE_SHIM" 2>/dev/null || cp "$QMAKE_QT5" "$QMAKE_SHIM"
fi

# ------------------------------------------------------------
# Deploy Qt runtime
# ------------------------------------------------------------
echo "==> Running windeployqt"
windeployqt-qt5 \
  --release \
  --no-angle \
  --no-opengl-sw \
  --no-translations \
  --no-compiler-runtime \
  "${DIST_DIR}/${APP_NAME}.exe"

# ------------------------------------------------------------
# Adding MinGW runtime DLLs
# ------------------------------------------------------------
echo "[INFO] Bundling MinGW runtime DLLs"
ldd "${DIST_DIR}/${APP_NAME}" | awk '{print $3}' | while read -r path; do
  case "$path" in
    /mingw64/bin/*.dll)
      dll="$(basename "$path")"
      echo "  + $dll"
      cp -n "$path" "${DIST_DIR}/"
      ;;
  esac
done

# ------------------------------------------------------------
# Optional: strip binary (smaller installer)
# ------------------------------------------------------------
strip "${DIST_DIR}/${APP_NAME}.exe" || true

# ------------------------------------------------------------
# Build NSIS installer
# ------------------------------------------------------------
echo "==> Building NSIS installer"

VERSION="dev"
if git describe --tags --exact-match >/dev/null 2>&1; then
  VERSION="$(git describe --tags --exact-match | sed 's/^v//')"
fi

makensis \
  -DAPP_NAME="Microkinetic Network Editor" \
  -DAPP_SHORT_NAME="MNE" \
  -DAPP_EXE="${APP_NAME}.exe" \
  -DVERSION="${VERSION}" \
  "microkinetic-network-editor.nsi"

echo "==> Done"
