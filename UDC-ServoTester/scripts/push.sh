#!/usr/bin/env bash
# push.sh — bump sketch version, commit version files if needed, then git push.
#
# Sketch-local helper. Does not install a repo-wide hook.
# Run from anywhere; operates on the UDC git root that contains this sketch.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO="$(cd "${SKETCH}/.." && pwd)"

VERSION_FILES=(
  "UDC-ServoTester/UDC-ServoTester.ino"
  "UDC-ServoTester/WebUi.h"
  "UDC-ServoTester/README.md"
)

cd "${REPO}"

echo "[push] bumping version…"
"${SCRIPT_DIR}/bump-version.sh" push
NEW_VER="$("${SCRIPT_DIR}/bump-version.sh" show)"

dirty=0
for f in "${VERSION_FILES[@]}"; do
  if ! git diff --quiet -- "${f}" 2>/dev/null; then
    dirty=1
    break
  fi
done

if (( dirty )); then
  git add -- "${VERSION_FILES[@]}"
  # Only create a version-only commit when those files are the only staged changes
  # relative to HEAD for this path set; other unstaged work is left alone.
  git commit -m "chore(UDC-ServoTester): bump version to ${NEW_VER}" -- "${VERSION_FILES[@]}"
  echo "[push] committed version bump → ${NEW_VER}"
else
  echo "[push] version files already match working tree / no bump delta to commit"
fi

echo "[push] git push…"
git push "$@"
echo "[push] done (version ${NEW_VER})"
