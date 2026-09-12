#!/usr/bin/env bash
set -euo pipefail

MMS_PATH="${1:-${MMSOURCE20:-}}"
if [[ -z "$MMS_PATH" ]]; then
  echo "Usage: $0 /path/to/metamod-source" >&2
  exit 2
fi

SAMPLE="$MMS_PATH/samples/s2_sample_mm"
for file in AMBuildScript configure.py PackageScript; do
  if [[ ! -f "$SAMPLE/$file" ]]; then
    echo "Missing $SAMPLE/$file" >&2
    exit 3
  fi
  cp "$SAMPLE/$file" "$(dirname "$0")/../$file"
done

echo "Synced AMBuild scripts from $SAMPLE"
