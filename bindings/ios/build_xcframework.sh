#!/usr/bin/env bash
# iOS xcframework 빌드: device(arm64) + simulator(arm64+x86_64)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT/build"
OUT="$ROOT/bindings/ios/TradeChartEngine.xcframework"

cd "$ROOT"

echo "[1/4] Configure ios-device"
cmake --preset ios-device -DTCE_BUILD_TESTS=OFF >/dev/null

echo "[2/4] Build ios-device"
cmake --build "$BUILD_DIR/ios-device" --config Release --target tce >/dev/null

echo "[3/4] Configure & build ios-sim"
cmake --preset ios-sim -DTCE_BUILD_TESTS=OFF >/dev/null
cmake --build "$BUILD_DIR/ios-sim" --config Release --target tce >/dev/null

echo "[4/4] Create xcframework"
DEVICE_LIB="$BUILD_DIR/ios-device/core/Release-iphoneos/libtce.a"
SIM_LIB="$BUILD_DIR/ios-sim/core/Release-iphonesimulator/libtce.a"

if [[ ! -f "$DEVICE_LIB" || ! -f "$SIM_LIB" ]]; then
    echo "Error: build artifacts missing"
    echo "  device: $DEVICE_LIB"
    echo "  sim:    $SIM_LIB"
    exit 1
fi

# Swift import 가능하게 module.modulemap 포함된 헤더 디렉토리 구성
HEADERS_PACKAGED="$BUILD_DIR/ios-headers"
rm -rf "$HEADERS_PACKAGED"
mkdir -p "$HEADERS_PACKAGED/tce"
cp "$ROOT"/core/include/tce/*.h "$HEADERS_PACKAGED/tce/"
cat > "$HEADERS_PACKAGED/module.modulemap" << 'EOF'
module TradeChartEngineC {
    umbrella header "tce/tce.h"
    header "tce/types.h"
    export *
}
EOF

rm -rf "$OUT"
xcodebuild -create-xcframework \
    -library "$DEVICE_LIB" -headers "$HEADERS_PACKAGED" \
    -library "$SIM_LIB"    -headers "$HEADERS_PACKAGED" \
    -output "$OUT"

echo ""
echo "✔ TradeChartEngine.xcframework 생성: $OUT"
