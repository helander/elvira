#!/bin/bash
# The elvira program is not very strict on the order of the program command line parameters,
# however this service script requires the name and uri parameters to come first (in that order),
# since the script uses these values for the service name and description.

name=$1
uri=$2

systemd-run --user \
  --unit="elvira.${name}" \
  --description="elvira engine job ($name) ($uri)" \
  --property=WorkingDirectory=$HOME \
  --property=Environment="XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR}" \
  --property=Environment="LV2_PATH=${HOME}/.lv2:/usr/lib/lv2" \
  --property=Environment="DISPLAY=${DISPLAY}" \
  elvira $@
