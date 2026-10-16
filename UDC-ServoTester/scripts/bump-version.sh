#!/usr/bin/env bash
# bump-version.sh — ISO-week versioning for UDC-ServoTester
#
# Scheme: YYYY.MM-WW.NN  (example: 2026.10-42.01)
#   YYYY = calendar year
#   MM   = calendar month
#   WW   = ISO week number (date +%V / isocalendar week)
#   NN   = build within that week
#
# Rules:
#   1. If today's YYYY.MM-WW ≠ version prefix → YYYY.MM-WW.01
#   2. Else (same week) → increment .NN
#   3. Week rollover already sets .01 — do not also increment
#
# Usage:
#   ./scripts/bump-version.sh              # same as auto
#   ./scripts/bump-version.sh auto         # week rollover or increment
#   ./scripts/bump-version.sh push         # same as auto (for pre-push / push.sh)
#   ./scripts/bump-version.sh date         # force today's YYYY.MM-WW.01
#   ./scripts/bump-version.sh set <ver>    # set exact version
#   ./scripts/bump-version.sh show         # print current version

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

INO="${ROOT}/UDC-ServoTester.ino"
WEBUI="${ROOT}/WebUi.h"
README="${ROOT}/README.md"

VERSION_RE='^[0-9]{4}\.[0-9]{2}-[0-9]{2}\.[0-9]{2}$'

die() {
  echo "error: $*" >&2
  exit 1
}

# Calendar year.month + ISO week → e.g. 2026.10-42
today_week() {
  date +%Y.%m-%V
}

read_version() {
  [[ -f "${INO}" ]] || die "missing ${INO}"
  local ver
  ver="$(sed -n 's/^#define UDC_SERVO_TESTER_VERSION "\([^"]*\)".*/\1/p' "${INO}" | head -n1)"
  [[ -n "${ver}" ]] || die "could not read UDC_SERVO_TESTER_VERSION from ${INO}"
  [[ "${ver}" =~ ${VERSION_RE} ]] || die "invalid current version '${ver}' (want YYYY.MM-WW.NN)"
  printf '%s\n' "${ver}"
}

validate_version() {
  local ver="$1"
  [[ "${ver}" =~ ${VERSION_RE} ]] || die "invalid version '${ver}' (want YYYY.MM-WW.NN, e.g. 2026.10-42.01)"
}

version_week() {
  # 2026.10-42.01 → 2026.10-42
  printf '%s\n' "${1%.*}"
}

version_nn() {
  # 2026.10-42.01 → 01
  printf '%s\n' "${1##*.}"
}

inc_nn() {
  local nn="$1"
  local n
  n=$((10#${nn} + 1))
  if (( n > 99 )); then
    die "build number would exceed 99 (${nn} → ${n})"
  fi
  printf '%02d\n' "${n}"
}

compute_auto() {
  local cur="$1"
  local today
  today="$(today_week)"
  local cur_week
  cur_week="$(version_week "${cur}")"
  if [[ "${today}" != "${cur_week}" ]]; then
    printf '%s.01\n' "${today}"
  else
    printf '%s.%s\n' "${cur_week}" "$(inc_nn "$(version_nn "${cur}")")"
  fi
}

compute_date() {
  # Forces today's week .01 (name kept for CLI compat; means "reset to this week")
  printf '%s.01\n' "$(today_week)"
}

apply_version() {
  local new="$1"
  local old="$2"
  validate_version "${new}"

  if [[ "${new}" == "${old}" ]]; then
    echo "version unchanged: ${new}"
    return 0
  fi

  # .ino define (Serial banner uses kSketchVersion — no separate hardcode)
  if grep -q "#define UDC_SERVO_TESTER_VERSION \"${old}\"" "${INO}"; then
    sed -i.bak "s/#define UDC_SERVO_TESTER_VERSION \"${old}\"/#define UDC_SERVO_TESTER_VERSION \"${new}\"/" "${INO}"
    rm -f "${INO}.bak"
  else
    die "define for '${old}' not found in ${INO}"
  fi

  # WebUi.h — any vOLD or >OLD< leftover labels
  if [[ -f "${WEBUI}" ]]; then
    sed -i.bak \
      -e "s/>${old}</>${new}</g" \
      -e "s/v${old}/v${new}/g" \
      "${WEBUI}"
    rm -f "${WEBUI}.bak"
    # Also rewrite footer ver span if it still carries a different stamped version
    if ! grep -q "v${new}" "${WEBUI}"; then
      sed -i.bak -E "s/(UDC Servo &amp; LED Tester · v)[0-9]{4}\.[0-9]{2}-[0-9]{2}\.[0-9]{2}/\1${new}/" "${WEBUI}"
      rm -f "${WEBUI}.bak"
    fi
  fi

  # README.md — **Version:** `…`
  if [[ -f "${README}" ]]; then
    if grep -q "\*\*Version:\*\* \`${old}\`" "${README}"; then
      sed -i.bak "s/\*\*Version:\*\* \`${old}\`/\*\*Version:\*\* \`${new}\`/" "${README}"
    else
      sed -i.bak -E "s/(\*\*Version:\*\* \`)[0-9]{4}\.[0-9]{2}-[0-9]{2}\.[0-9]{2}(\`)/\1${new}\2/" "${README}"
    fi
    rm -f "${README}.bak"
  fi

  echo "bumped ${old} → ${new}"
  echo "updated: UDC-ServoTester.ino WebUi.h README.md"
}

usage() {
  cat <<'EOF'
Usage: bump-version.sh [auto|push|date|show|set <version>]

  auto / push  Week rollover → YYYY.MM-WW.01; same ISO week → increment .NN
  date         Force today's YYYY.MM-WW.01 (ISO week)
  set <ver>    Set exact version (YYYY.MM-WW.NN)
  show         Print current version

Scheme: YYYY.MM-WW.NN — WW is ISO week (date +%V). Month is calendar month.
EOF
}

main() {
  local mode="${1:-auto}"
  local cur new

  case "${mode}" in
    -h|--help|help)
      usage
      exit 0
      ;;
    show)
      read_version
      exit 0
      ;;
    auto|push)
      cur="$(read_version)"
      new="$(compute_auto "${cur}")"
      apply_version "${new}" "${cur}"
      ;;
    date)
      cur="$(read_version)"
      new="$(compute_date)"
      apply_version "${new}" "${cur}"
      ;;
    set)
      [[ $# -ge 2 ]] || die "set requires a version argument"
      cur="$(read_version)"
      new="$2"
      validate_version "${new}"
      apply_version "${new}" "${cur}"
      ;;
    *)
      usage
      die "unknown mode '${mode}'"
      ;;
  esac
}

main "$@"
