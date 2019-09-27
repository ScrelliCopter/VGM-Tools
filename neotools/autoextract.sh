#!/bin/sh

FILE="$1"
NAME="$(basename "$FILE")"
WAVDIR="${NAME%%.*}"

if [ "${NAME##*.}" = "vgz" ]; then
	cp "$FILE" "temp.vgm.gz"
	gzip -d "temp.vgm.gz"
	FILE="temp.vgm"
fi

./neoadpcmextract "$FILE"
mkdir -p "$WAVDIR"
for I in smpa_*.pcm; do ./adpcm "$I" "$WAVDIR/${I%%.*}.wav"; done
for I in smpb_*.pcm; do ./adpcmb -d "$I" "$WAVDIR/${I%%.*}.wav"; done
find . -type f -name "*.pcm" -exec rm -f {} \;

[ "$FILE" = "temp.vgm" ] && rm -f "temp.vgm"
