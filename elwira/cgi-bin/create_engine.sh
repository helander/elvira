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

logfile=/tmp/elvira-${name}.log
rm -f ${logfile}
nohup elvira --showui ${name} ${uri} > ${logfile}  &
# Remove the logfile once the elvira process terminates
# the content is until then accessable via "cat /proc/<pid>/fd/1"
unlink ${logfile}
