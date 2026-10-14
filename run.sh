#!/bin/sh
# Runs the game from the build with your disc image (WOC_DISC or local.json's path), passing the options on.
DISC="${WOC_DISC:-$(python3 -c 'import json;print(json.load(open("local.json"))["disc"])' 2>/dev/null)}"
APP="$(dirname "$0")/build/crash-woc.app/Contents/MacOS/crash-woc"
[ -x "$APP" ] || APP="$(dirname "$0")/build/crash-woc"
exec "$APP" --disc "$DISC" "$@"
