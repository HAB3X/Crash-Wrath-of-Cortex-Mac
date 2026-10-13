#!/bin/sh
# Runs the game from the build with your disc image (WOC_DISC or local.json's path), passing the options on.
DISC="${WOC_DISC:-$(python3 -c 'import json;print(json.load(open("local.json"))["disc"])' 2>/dev/null)}"
exec "$(dirname "$0")/build/crash-woc" --disc "$DISC" "$@"
