#!/bin/bash
echo "Content-Type: text/plain"
echo

# Get the raw query string
QUERY="$QUERY_STRING"

# URL decode function
urldecode() {
  local data="${1//+/ }"
  printf '%b' "${data//%/\\x}"
}

# Parse into key=value pairs
declare -A params
IFS='&' read -ra pairs <<< "$QUERY"
for pair in "${pairs[@]}"; do
  IFS='=' read -r key value <<< "$pair"
  key=$(urldecode "$key")
  value=$(urldecode "$value")
  params["$key"]="$value"
done

name=${params["name"]}
uri=${params["uri"]}

systemd-run --user \
  --unit="elvira.${name}" \
  --description="elvira engine job ($name) ($uri)" \
  --property=WorkingDirectory=$HOME \
  --property=Environment="XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR}" \
  --property=Environment="LV2_PATH=${LV2_PATH}" \
  --property=Environment="DISPLAY=${DISPLAY}" \
  --property=Environment="PIPEWIRE_DEBUG=4" \
  elvira --showui  $name $uri
