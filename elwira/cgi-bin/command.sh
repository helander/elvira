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
command=$(echo "$JSON_DATA" | jq -r '.command')
node_id=$(echo "$JSON_DATA" | jq -r '.node_id')
preset_uri=$(echo "$JSON_DATA" | jq -r '.preset_uri')

# Output the parsed values
echo "Parsed values:"
echo "Command: $command"
echo "Node id: $node_id"
echo "Preset uri: $preset_uri"
command_string="$command $preset_uri"
echo "command_string: $command_string"
extra_string="extra=\"$command_string\""
echo "extra_string: $extra_string"
pw_string="pw-cli send-command $node_id User {$extra_string}"
echo "pw_string: $pw_string"
$pw_string

#pw-cli send-command $node_id $object_string
