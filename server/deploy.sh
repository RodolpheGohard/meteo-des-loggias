#!/usr/bin/env bash
# Deploys server/index.js to the register-sensors-data Cloud Function.
#
# The function's source root is flat (index.js, package.json, apikey.json,
# and the service account key all sit next to each other), while in this repo
# apikey.json and package.json live at the repo root, one level above
# server/. This script stages a temporary flat copy with the require() path
# rewritten to match, deploys from there, and cleans up afterwards.
set -euo pipefail

FUNCTION_NAME=register-sensors-data
REGION=europe-west1
RUNTIME=nodejs26
ENTRY_POINT=helloHttp

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SA_KEY_FILE="$(find "$SCRIPT_DIR" -maxdepth 1 -name 'meteo-des-loggias-*.json' | head -n1)"
if [[ -z "$SA_KEY_FILE" ]]; then
  echo "error: no service account key (meteo-des-loggias-*.json) found in $SCRIPT_DIR" >&2
  exit 1
fi

STAGE_DIR="$(mktemp -d)"
trap 'rm -rf "$STAGE_DIR"' EXIT

cp "$SCRIPT_DIR/index.js" "$STAGE_DIR/index.js"
cp "$REPO_ROOT/package.json" "$STAGE_DIR/package.json"
cp "$REPO_ROOT/apikey.json" "$STAGE_DIR/apikey.json"
cp "$SA_KEY_FILE" "$STAGE_DIR/$(basename "$SA_KEY_FILE")"
sed -i 's#require("\.\./apikey\.json")#require("./apikey.json")#' "$STAGE_DIR/index.js"

echo "Deploying $FUNCTION_NAME ($REGION, $RUNTIME) from $STAGE_DIR ..."
gcloud functions deploy "$FUNCTION_NAME" \
  --gen2 \
  --runtime="$RUNTIME" \
  --region="$REGION" \
  --source="$STAGE_DIR" \
  --entry-point="$ENTRY_POINT" \
  --trigger-http

echo "Done."
