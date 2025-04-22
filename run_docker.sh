#!/usr/bin/env bash
set -euo pipefail

# Absolute path of the directory that contains this script
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 1 – Configure virtual CAN interface (vcan0); only this step needs sudo
echo "🔧 Configuring virtual CAN interface (vcan0)…"
sudo bash "$SCRIPT_DIR/setup_vcan.sh"

# 2 – Build all Docker images in parallel
echo "🚢 Building Docker images…"
docker-compose build --parallel

# 3 – List of ECU services to start
services=(ecu_dashboard ecu_bcm ecu_powertrain ecu_instrument_cluster)

# 4 – Launch each ECU in its own GNOME Terminal window
for svc in "${services[@]}"; do
    echo "➡️  Starting $svc in a new terminal…"
    gnome-terminal --geometry 92x22 \
        -- bash -c "cd '$SCRIPT_DIR' && docker-compose up $svc; exec bash"
done

echo "✅ All ECUs are running in separate terminals!"
