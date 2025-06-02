#!/bin/bash
echo "Content-Type: application/json"
echo

pw-dump | jq -r '[.[]
    | select(.type == "PipeWire:Interface:Node") 
    | select(.info.props["elvira.role"] == "engine") 
    | {id: .id, name: .info.props["node.name"], set: .info.props["elvira.set"], plugin: .info.props["elvira.plugin"], preset: .info.props["elvira.preset"]}
  ]'

