#!/bin/bash
echo "Content-Type: text/plain"
echo ""

# Read raw JSON input from stdin
read -n "$CONTENT_LENGTH" JSON_DATA

# Print raw input (for debug)
echo "Raw JSON:"
echo "$JSON_DATA"
echo ""

# Check if jq is installed
if ! command -v jq > /dev/null; then
  echo "Error: jq not found"
  exit 1
fi

# Parse JSON using jq
key=$(echo "$JSON_DATA" | jq -r '.key')
node_id=$(echo "$JSON_DATA" | jq -r '.node_id')
value=$(echo "$JSON_DATA" | jq -r '.value')

pw-metadata $node_id $key $value
