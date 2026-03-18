#!/usr/bin/env bash
set -e

LOGFILE="$1"

if [ -f "$LOGFILE" ]; then
    ERRORS=$(grep -E "warning:" "$LOGFILE" || true)
    if [ -n "$ERRORS" ]; then
        echo "❌ Documentation lint failed:"
        echo "$ERRORS"
        exit 1
    fi
fi

echo "✅ Documentation lint passed"