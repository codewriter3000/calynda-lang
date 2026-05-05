#!/usr/bin/env bash
# Splits a C source/header file into ≤250-line parts via #include fragments.
# Cuts are snapped to safe boundaries: blank lines, '}' lines, or '    },' (struct/array entry endings).
# For .h files: uses suffix _hp (vs .c's _p) to avoid collisions with same-stem .c, and
#               keeps the include-guard #endif in the head file.
set -euo pipefail

FILE=$1
DIR=$(dirname "$FILE")
EXT="${FILE##*.}"
BASE=$(basename "$FILE" ".$EXT")
N=$(wc -l < "$FILE")
LIMIT=245

if (( N <= 250 )); then echo "skip $FILE ($N lines)"; exit 0; fi

if [[ "$EXT" == "h" ]]; then SUFFIX_PREFIX="hp"; else SUFFIX_PREFIX="p"; fi

GUARD_ENDIF=0
if [[ "$EXT" == "h" ]]; then
  GUARD_ENDIF=$(grep -nE "^\s*#\s*endif" "$FILE" | tail -1 | cut -d: -f1 || true)
  GUARD_ENDIF=${GUARD_ENDIF:-0}
fi

BODY_END=$N
if (( GUARD_ENDIF > 0 )); then BODY_END=$(( GUARD_ENDIF - 1 )); fi

mapfile -t BLANKS < <(awk '/^\s*$/ || /^\}\s*$/ || /^[[:space:]]+\},?[[:space:]]*$/ {print NR}' "$FILE")

cuts=()
start=1
while (( start + LIMIT < BODY_END )); do
  target=$(( start + LIMIT ))
  best=0
  for ln in "${BLANKS[@]}"; do
    if (( ln > start && ln <= target && ln < BODY_END )); then best=$ln
    elif (( ln > target )); then break; fi
  done
  if (( best == 0 )); then
    echo "ERROR: no cut point for $FILE between $start and $target" >&2; exit 1
  fi
  cuts+=("$best"); start=$(( best + 1 ))
done

if (( ${#cuts[@]} == 0 )); then echo "skip $FILE"; exit 0; fi

prev=1; idx=2
HEAD_TMP=$(mktemp)
for c in "${cuts[@]}"; do
  if (( prev == 1 )); then
    sed -n "${prev},${c}p" "$FILE" > "$HEAD_TMP"
    echo "#include \"${BASE}_${SUFFIX_PREFIX}${idx}.inc\"" >> "$HEAD_TMP"
  else
    out="$DIR/${BASE}_${SUFFIX_PREFIX}${idx_prev}.inc"
    sed -n "${prev},${c}p" "$FILE" > "$out"
    echo "#include \"${BASE}_${SUFFIX_PREFIX}${idx}.inc\"" >> "$out"
  fi
  idx_prev=$idx; idx=$(( idx + 1 )); prev=$(( c + 1 ))
done

out="$DIR/${BASE}_${SUFFIX_PREFIX}${idx_prev}.inc"
sed -n "${prev},${BODY_END}p" "$FILE" > "$out"

if (( GUARD_ENDIF > 0 )); then
  sed -n "${GUARD_ENDIF},${N}p" "$FILE" >> "$HEAD_TMP"
fi

mv "$HEAD_TMP" "$FILE"
echo "split $FILE:"
wc -l "$FILE" "$DIR/${BASE}_${SUFFIX_PREFIX}"*.inc
