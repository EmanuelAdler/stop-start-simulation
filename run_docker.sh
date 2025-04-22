#!/bin/bash

# Run the CAN setup script
bash "setup_vcan.sh"

# Build all services first
echo "Building Docker images..."
docker-compose build

# ecu_dashboard will only run with "docker-compose run"
echo "Starting ecu_dashboard in a new terminal..."
gnome-terminal --geometry 92x22 -- bash -c "docker-compose run ecu_dashboard; exec bash"

# List of other ECUs to run
services=("ecu_bcm" "ecu_powertrain" "ecu_instrument_cluster")

# Launch each ECU in a new terminal
for service in "${services[@]}"; do
    echo "Starting $service in a new terminal..."
    gnome-terminal -- bash -c "cd '$SCRIPT_DIR' && docker-compose up $service; exec bash"
    # macOS alternative:
    # osascript -e "tell app \"Terminal\" to do script \"cd '$SCRIPT_DIR' && docker-compose up $service\""
done

echo "All ECUs are running in separate terminals!"