#!/bin/bash
echo "Content-Type: application/json"
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


lv2info ${params["uri"]} 2>&1|grep ^Preset|awk -F '<|>' '{print $2}'|jq -R -s 'split("\n")[:-1]'
