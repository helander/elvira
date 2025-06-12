#!/bin/bash

# Read raw JSON input from stdin
read -n "$CONTENT_LENGTH" JSON_DATA


# Check if jq is installed
if ! command -v jq > /dev/null; then
  echo "Error: jq not found"
  exit 1
fi

# Parse JSON using jq
node_id=$(echo "$JSON_DATA" | jq -r '.node_id')

echo "Content-Type: application/json"
echo ""

pw-dump | jq --argjson n $node_id -r '.[] | select(.id == $n and .type == "PipeWire:Interface:Node") | .info.props."elvira.host.info.base"|fromjson'
