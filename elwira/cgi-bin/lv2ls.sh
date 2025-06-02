#!/bin/bash
echo "Content-Type: application/json"
echo

lv2ls|jq -R -s 'split("\n")[:-1]' 
