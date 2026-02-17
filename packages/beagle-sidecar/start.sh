#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$SCRIPT_DIR"
BIN="$REPO_DIR/build/beagle-sidecar"

usage() {
  cat <<'EOF'
Usage:
  ./start.sh run
  ./start.sh --install-systemd-user
  ./start.sh --uninstall-systemd-user
  ./start.sh --status-systemd-user

Environment:
  BEAGLE_SDK_ROOT            Required. SDK root containing config/carrier.conf
  BEAGLE_SIDECAR_DATA_DIR    Default: ~/.carrier
  BEAGLE_SIDECAR_PORT        Optional port override
  BEAGLE_SIDECAR_TOKEN       Optional bearer token
  BEAGLE_SIDECAR_EXTRA_ARGS  Optional extra args (space-separated)
EOF
}

resolve_sdk_root() {
  if [[ -n "${BEAGLE_SDK_ROOT:-}" ]]; then
    return 0
  fi
  local cache="$REPO_DIR/build/CMakeCache.txt"
  if [[ -f "$cache" ]]; then
    local line
    line="$(rg -N "BEAGLE_SDK_ROOT:UNINITIALIZED=" "$cache" || true)"
    if [[ -n "$line" ]]; then
      BEAGLE_SDK_ROOT="${line#*=}"
      export BEAGLE_SDK_ROOT
    fi
  fi
}

ensure_sdk_root() {
  resolve_sdk_root
  if [[ -z "${BEAGLE_SDK_ROOT:-}" ]]; then
    echo "BEAGLE_SDK_ROOT is required (or build/CMakeCache.txt must include it)." >&2
    exit 1
  fi
}

build_args() {
  local data_dir="${BEAGLE_SIDECAR_DATA_DIR:-$HOME/.carrier}"
  ARGS=(--config "$BEAGLE_SDK_ROOT/config/carrier.conf" --data-dir "$data_dir")
  if [[ -n "${BEAGLE_SIDECAR_PORT:-}" ]]; then
    ARGS+=(--port "$BEAGLE_SIDECAR_PORT")
  fi
  if [[ -n "${BEAGLE_SIDECAR_TOKEN:-}" ]]; then
    ARGS+=(--token "$BEAGLE_SIDECAR_TOKEN")
  fi
  if [[ -n "${BEAGLE_SIDECAR_EXTRA_ARGS:-}" ]]; then
    read -r -a EXTRA_ARGS <<<"$BEAGLE_SIDECAR_EXTRA_ARGS"
    ARGS+=("${EXTRA_ARGS[@]}")
  fi
}

cmd="${1:-run}"
case "$cmd" in
  run)
    ensure_sdk_root
    build_args
    export LD_LIBRARY_PATH="${BEAGLE_SDK_ROOT}/build/linux/src/carrier:${BEAGLE_SDK_ROOT}/build/linux/src/session:${BEAGLE_SDK_ROOT}/build/linux/src/filetransfer:${BEAGLE_SDK_ROOT}/build/linux/intermediates/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    exec "$BIN" "${ARGS[@]}"
    ;;
  --install-systemd-user)
    "$REPO_DIR/scripts/setup-systemd-user.sh" install
    ;;
  --uninstall-systemd-user)
    "$REPO_DIR/scripts/setup-systemd-user.sh" uninstall
    ;;
  --status-systemd-user)
    "$REPO_DIR/scripts/setup-systemd-user.sh" status
    ;;
  -h|--help)
    usage
    ;;
  *)
    echo "Unknown command: $cmd" >&2
    usage
    exit 1
    ;;
esac
