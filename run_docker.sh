#!/bin/bash

# Run the CAN setup script
bash "setup_vcan.sh"

# Build all services first
echo "Building Docker images..."
docker-compose build

# List of other ECUs to run
services=("ecu_bcm" "ecu_powertrain" "ecu_instrument_cluster")

# Launch each ECU in a new terminal
for service in "${services[@]}"; do
    echo "Starting $service in a new terminal..."
    gnome-terminal -- bash -c "cd '$SCRIPT_DIR' && docker-compose up $service; exec bash"
    # macOS alternative:
    # osascript -e "tell app \"Terminal\" to do script \"cd '$SCRIPT_DIR' && docker-compose up $service\""
done

# ecu_dashboard will only run with "docker-compose run"
gnome-terminal --geometry 94x22 -- bash -c "docker-compose run ecu_dashboard; exec bash"

echo "All ECUs are running in separate terminals!"