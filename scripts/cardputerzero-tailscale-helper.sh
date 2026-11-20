#!/bin/sh
# SPDX-FileCopyrightText: 2026 Ashish
# SPDX-License-Identifier: MIT
#
# Fixed-verb wrapper around the real `tailscale` binary. This script is the
# ONLY thing a sudoers NOPASSWD rule should ever point at for this app --
# never grant NOPASSWD on `tailscale` itself with a wildcard, since sudoers
# wildcard matching on arbitrary arguments is not a safe boundary.
#
# Install at /usr/local/bin/cardputerzero-tailscale-helper (root:root 0755),
# then see docs/TAILSCALE_SETUP.md for the matching sudoers drop-in.
#
# Usage:
#   cardputerzero-tailscale-helper.sh status
#   cardputerzero-tailscale-helper.sh up
#   cardputerzero-tailscale-helper.sh up-with-key <authkey>
#   cardputerzero-tailscale-helper.sh down

set -eu

TAILSCALE_BIN="$(command -v tailscale)"

if [ -z "${TAILSCALE_BIN}" ]; then
    echo "tailscale binary not found on PATH" >&2
    exit 127
fi

case "${1:-}" in
    status)
        exec "${TAILSCALE_BIN}" status
        ;;
    up)
        exec "${TAILSCALE_BIN}" up
        ;;
    up-with-key)
        if [ -z "${2:-}" ]; then
            echo "up-with-key requires an auth key argument" >&2
            exit 64
        fi
        exec "${TAILSCALE_BIN}" up --authkey="${2}"
        ;;
    down)
        exec "${TAILSCALE_BIN}" down
        ;;
    *)
        echo "unknown verb: ${1:-<none>}" >&2
        echo "usage: $0 {status|up|up-with-key <key>|down}" >&2
        exit 64
        ;;
esac
