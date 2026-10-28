#!/bin/sh
# Builds a universal Release and packages it as an ad-hoc signed .dmg
set -e
cd "$(dirname "$0")/.."

cmake -B build -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build build --config Release -j

APP="build/ProjectionMask_artefacts/Release/Projection Mask.app"
VERSION=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$APP/Contents/Info.plist")
DMG="ProjectionMask-$VERSION.dmg"

codesign --force --deep --sign - "$APP"

STAGE=$(mktemp -d)
cp -R "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"

rm -f "$DMG"
hdiutil create -volname "Projection Mask" -srcfolder "$STAGE" -ov -format UDZO "$DMG"
rm -rf "$STAGE"

echo "Created $DMG"
