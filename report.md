# Source Code Management (SCM) Report

## 1. Branching Strategy
For this project, we adopted the **Git Flow** branching model, ensuring an organized and structured development workflow. The key branches are:

- **`main`**: The production-ready branch, containing stable releases.
- **`develop`**: The integration branch where features are merged before being released.
- **`feature/<feature-name>`**: Created for each new feature or enhancement, based on `develop`.
- **`hotfix/<fix-name>`**: Used for urgent bug fixes in `main`, merged into both `main` and `develop`.
- **`release/<version>`**: Used to finalize a release, allowing last-minute testing and documentation before merging into `main`.

Merging to `main` is only done through Pull Requests (PRs), ensuring code reviews and automated checks before deployment.

## 2. Build and CI/CD Procedures
We implemented a **Continuous Integration (CI)** and **Continuous Deployment (CD)** pipeline using GitHub Actions. The pipeline consists of:

### **CI Process:**
1. Linting using **MISRA-C** to enforce coding standards.
2. Running **unit tests** automatically upon PR creation.
3. Running **integration tests** on every PR to `develop`.
4. Building the Docker container to ensure successful compilation.

### **CD Process:**
1. Every merge to `main` triggers an **automated build**.
2. Docker images are built and pushed to a container registry.
3. A deployment script is executed to update the live environment.

## 3. Test Structure and Policy
Testing is a critical part of our workflow, consisting of:

- **Unit Tests:** Implemented for core logic using a modular approach with `.h` and `.c` files.
- **Integration Tests:** Ensure that different modules interact correctly.
- **Automated CI Tests:** Run on every PR.

Each PR must pass all tests before being merged into `develop` or `main`.

## 4. Versioning System
We follow **Semantic Versioning (SemVer)**, with releases tagged as:
- `vMAJOR.MINOR.PATCH` (e.g., `v1.2.3`).
- **Major**: Significant changes or breaking updates.
- **Minor**: New features maintaining backward compatibility.
- **Patch**: Bug fixes and small improvements.

Each release is documented using GitHub **tags and releases**, ensuring clear history tracking.

## 5. Issue Management
We use GitHub Issues to track tasks and bugs, following this **lifecycle:**

1. **New Issue:** Labeled based on category (`bug`, `feature`, `documentation`, etc.).
2. **Assigned:** A team member is responsible for resolving the issue.
3. **In Progress:** Work is done on a separate feature/hotfix branch.
4. **Review:** A PR is created and reviewed by at least one team member.
5. **Merged & Closed:** The branch is merged into `develop` or `main`, and the issue is closed.

## 6. Project Management with GitHub Projects
To efficiently track progress, we utilize **GitHub Projects** to organize and prioritize issues and tasks. The project board is divided into the following columns:

- **Backlog:** Ideas, feature requests, and bug reports yet to be prioritized.
- **To Do:** Tasks selected for development in the current sprint.
- **In Progress:** Actively being worked on.
- **Review:** Awaiting code review and approval before merging.
- **Done:** Completed and merged tasks.

Each issue is linked to a project board item to provide clear visibility of progress and streamline collaboration.

## 7. Commit Convention
To maintain consistency and clarity in our version history, we follow a structured **commit message convention**:

- **feat:** A new feature (e.g., `feat: add authentication system`)
- **fix:** A bug fix (e.g., `fix: resolve crash on login`)
- **docs:** Documentation changes (e.g., `docs: update README`)
- **chore:** Maintenance changes that do not affect functionality (e.g., `chore: update dependencies`)
- **test:** Adding or updating tests (e.g., `test: add unit test for API`)
- **refactor:** Code changes that do not add features or fix bugs (e.g., `refactor: optimize loop in processor`)
- **ci:** CI/CD configuration changes (e.g., `ci: update GitHub Actions workflow`)

This ensures that every commit provides meaningful context and can be easily tracked.
