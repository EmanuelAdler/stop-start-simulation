#!/bin/bash

bash "setup_vcan.sh"

services=("ecu_dashboard" "ecu_powertrain" "ecu_instrument_cluster" "ecu_bcm")

for service in "${services[@]}"; do
    gnome-terminal -- bash -c "docker-compose up $service; exec bash"
    # For macOS, use:
    # osascript -e "tell app \"Terminal\" to do script \"docker-compose up $service\""
done