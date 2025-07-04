#!/bin/bash
echo "Content-Type: application/json"
echo

pw-dump | jq -r '[.[]
    | select(.type == "PipeWire:Interface:Node")
    | select(.info.props["elvira.role"] == "instance")
    | {id: .id, name: .info.props["node.name"], gain: .info.props["elvira.gain"], plugin: .info.props["elvira.plugin"], preset: .info.props["elvira.preset"]}
  ]'

