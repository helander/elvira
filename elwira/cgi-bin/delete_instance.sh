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
node_id=$(echo "$JSON_DATA" | jq -r '.node_id')

# Output the parsed values
echo "Parsed values:"
echo "Node id: $node_id"
pw-cli destroy $node_id

#pw-cli send-command $node_id $object_string
