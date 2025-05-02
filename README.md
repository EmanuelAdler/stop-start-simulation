# Stop/Start (SS) Simulation

![Tests](https://github.com/EmanuelAdler/stop-start-simulation/actions/workflows/auto-tests.yml/badge.svg)
![Coverage](https://github.com/EmanuelAdler/stop-start-simulation/actions/workflows/coverage.yml/badge.svg)
![Static Analysis](https://github.com/EmanuelAdler/stop-start-simulation/actions/workflows/linting.yml/badge.svg)
![Deploy](https://github.com/EmanuelAdler/stop-start-simulation/actions/workflows/docker-build.yml/badge.svg)

This project consists of an application that simulates the behaviour of an automotive stop/start system.

## Project guidelines
The rules regarding this project management like commiting, creating issues and naming branches are contained in the [report](report.md) file.

## Requirements  
- Ubuntu or another Linux system compatible with SocketCAN and GCC 14
- Docker  

## CAN Interface Setup  
Give execution permission to the script:  
```sh
chmod +x setup_vcan.sh
```
Run the script to configure the interface:
```sh
sudo ./setup_vcan.sh
```

## Building and Running the Containers
Build and start the containers:
```sh
docker-compose up --build
```

Check the running containers:
```sh
docker ps
```

## Communication Test
Send a message via CAN:
```sh
echo -n "message" | docker exec -i instrument_cluster sh -c 'cat > /tmp/command_pipe'
```

To stop the containers:
```sh
docker-compose down
```

## Checking the logs
When the container is running, execute:
```sh
docker exec -it <container> cat /app/logs/diagnostics.log
```

## Local Linting with Pre-Commit & Clang-Tidy
To maintain code quality and ensure MISRA-C compliance, we use clang-tidy locally through pre-commit hooks. This lets you catch issues before pushing changes to GitHub.

## 1. Install `pre-commit`
Pre-commit is a Python-based tool. If you don’t already have it:
```sh
sudo apt-get update
sudo apt-get install python3-pip -y
pip3 install pre-commit
```

## 2. Install `clang-tidy` (if not already installed)
```sh
sudo apt-get update
sudo apt-get install clang-tidy -y
```

## 3. Give execution permission to the script
```sh
chmod +x run_clang_tidy.sh
```

## 4. Enable Pre-Commit in Your Repository
In the repository’s root folder, run:
```sh
pre-commit install
```
This installs the Git hook so that every time a commit is made, pre-commit executes clang-tidy on staged files.

## 5. Testing the Hook
Try committing a file with some intentional lint issues.
If clang-tidy finds problems, it will block the commit until the issues are resolved or bypassed.

## CI/CD Pipeline

Our project utilizes GitHub Actions to automate various aspects of development and deployment. Below is an overview of each workflow:

### 1. Label Triage Workflow

**Purpose:** Automatically labels new issues with "triage" to streamline issue management.

**Workflow File:** `.github/workflows/label-triage.yml`

**Trigger:** Activates when a new issue is opened.

**Key Steps:**
- **Add "triage" Label:** Uses the `actions/github-script` action to add the "triage" label to new issues.

### 2. Linting Workflow

**Purpose:** Ensures code quality by performing static analysis on project files using `Clang-Tidy`.

**Workflow File:** `.github/workflows/linting.yml`

**Triggers:** Runs on every push and pull request.

**Key Steps:**
- **Checkout Code:** Retrieves the latest code from the repository.
- **Install Clang-Tidy:** Installs the `Clang-Tidy` tool on the runner.
- **Run Clang-Tidy:** Executes `Clang-Tidy` with MISRA-C compliance settings to analyze the C source files. The workflow is configured to fail if any issues are detected (`--error-exitcode=1`).

### 3. Docker Build and Push Workflow

**Purpose:** Automates the building and pushing of Docker images for the project.

**Workflow File:** `.github/workflows/docker-build.yml`

**Triggers:**
- Pushes to the `main` and `develop` branches.
- Creation of version tags matching the pattern `v*.*.*`.

**Key Steps:**
- **Checkout Code:** Retrieves the latest code from the repository.
- **Set up Docker Buildx:** Initializes Docker Buildx for advanced build capabilities.
- **Log in to Docker Hub:** Authenticates to Docker Hub using credentials stored in repository secrets (`DOCKERHUB_USERNAME` and `DOCKERHUB_TOKEN`).
- **Build and Push Docker Image:** Builds the Docker image and pushes it to Docker Hub with appropriate tags:
  - `latest` for the `main` and `develop` branches.
  - The specific Git reference name (e.g., branch name or tag).

### 4. Automated Testing Workflow

**Purpose:** Runs unit and integration tests to validate CAN message transmission.

**Workflow File:** `.github/workflows/auto-tests.yml`

**Triggers:**
- Runs on every pull request to the `main` or the `develop` branch.

**Key Steps:**
- **Checkout Code:** Retrieves the latest code from the repository.
- **Install Dependencies:** Installs necessary packages, including `can-utils`, `libcunit1-dev`, and required kernel modules.
- **Set Up CAN Interface:** Executes `setup_vcan.sh` to configure the virtual CAN interface.
- **Run Tests:** Executes `make test` to compile and run unit and integration tests.

### 5. Coverage Report Workflow
**Purpose:** Automatically generates a test coverage report using GCC 14.2.0 and LCOV 2.3-1, including MC/DC and branch coverage.

**Workflow File:** `.github/workflows/coverage.yml`

**Triggers:**
- Runs on every push to the `main` branch.

**Key Steps:**
- **Install GCC 14.2.0 & LCOV 2.3-1:** Ensures full support for branch and MC/DC analysis.
- **Generate Coverage:** Uses `make coverage` to run tests and produce a report.
- **Publish to GitHub Pages:** The report is deployed to the `gh-pages` branch and can be accessed online.

📄 **Coverage Report URL:** [https://emanueladler.github.io/stop-start-simulation](https://emanueladler.github.io/stop-start-simulation)

## CAN Data Dictionary

| CAN ID (hex) | Nominal DLC | Message name | Producer (module) | Main consumer(s) | Payload layout (byte offset → signal) | Notes |
|--------------|------------|------------------------|-------------------|------------------|---------------------------------------|-------|
| **0x110** | 8 | **CAN_ID_SENSOR_READ** | BCM | Dashboard, Powertrain | 0–7 → encrypted block (16 B is split into two 8‑byte frames) | Carries *any* sensor string: `speed`, `in_temp`, `ex_temp`, `door`, `tilt`, `accel`, `brake`, `temp_set`, `batt_soc`, `batt_volt`, `engi_temp`, `gear`. |
| **0x111** | 8 | **CAN_ID_COMMAND** | Dashboard / BCM | Powertrain, ECU | Encrypted string – typical values: `press_start_stop`, `error_disabled` | Used for high‑level driver requests or safety shutdowns. |
| **0x101** | 8 | **CAN_ID_ERROR_DASH** | Powertrain / BCM | Dashboard | Encrypted error keyword – e.g. `error_battery`, `error_battery_drop` | Shown as warnings on the instrument cluster. |
| **0x7E0** | 8 | **CAN_ID_ECU_RESTART** | Powertrain | Dashboard | Encrypted keywords: `ENGINE OFF`, `RESTART`, `ABORT` | Implements stop‑start restart sequence. |
