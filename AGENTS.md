# AGENTS.md

Context file for AI agents working on service-fabric.

**Dual Format**: This file combines Category A (Operations Manual) and Category B (Context Guide) for comprehensive agent guidance.

## Project Overview

service-fabric is a C project using Makefile.

**Key Info:**
- **Primary Language:** C
- **Build System:** Makefile
- **Test Framework:** None detected
- **Total Files:** 19243
- **Test Files:** 3378
- **AI Readiness Score:** 93/100 (Agent-Optimized)

---

## 🚨 AI Policy & Operations

Extracted from CONTRIBUTING.md - operational constraints and procedures.

### AI Policy

- Issues related to Production workloads  **MUST NOT BE REPORTED** in this repository. Production support is out of scope of this repository's obejctives. To receive product  support, you must file a support request through below recommended channels.
- [POA](https://github.com/microsoft/Service-Fabric-POA) for issues related to **Patch Orcherstration Agent**.
- We welcome all forms of contributions from the community. Please read the following guidelines  to maximize the chances of your PR being merged.

### Key Requirements

- Before you submit a pull request, a bot will prompt you to sign the [Microsoft Contributor License Agreement](https://cla.microsoft.com/). This needs to be done only once for any Microsoft-sponsored open source project - if you've signed the Microsoft CLA for any project sponsored by Microsoft, then you are good to go for all the repos sponsored by Microsoft.
- **Important**: Note that there are **two** different CLAs commonly used by Microsoft projects: [Microsoft CLA](https://cla.microsoft.com/) and [.NET Foundation CLA](https://cla2.dotnetfoundation.org/). Service Fabric open source projects use the [Microsoft](https://cla.microsoft.com/) CLA. The .NET Foundation is treated as a separate entity, so if you've signed the .NET Foundation CLA in the past, you will still need to sign the Microsoft CLA to contribute to Service Fabric open source projects.

### Development Procedures

- Security issues and bugs should be reported privately, via email, to  **[Microsoft Security Response Center (MSRC)](secure@microsoft.com)**. You should receive a response within 24 hours. If for some reason you do not, please follow up via email to ensure we received your original message. Further information, including the MSRC PGP key, can be found in the [Security TechCenter](https://technet.microsoft.com/en-us/security/ff852094.aspx).
- For general "how-to" and guidance questions about using Service Fabric to build and run applications, please use [Stack Overflow](http://stackoverflow.com/questions/tagged/azure-service-fabric) tagged with `azure-service-fabric`.
- For any work related to setting up build, test, and CI for Service Fabric on GitHub, or for small patches or bug fixes, please open an issue for tracking purposes, but we generally don't need a discussion prior to opening a PR.
- Please be sure to follow the usual process for submitting PRs:



## 🏗️ Architecture & Context Guide

This section provides architectural context and agent-understanding for the codebase.

### Prerequisites

- **C:** 3.9+ (or applicable language version)
- **Package Manager:** pip or uv
- **Test Runner:** None detected



### Project Structure

```
service-fabric/
├── package.json
├── package.json
├── Makefile
├── src/                  # Source code
├── tests/                # Test suite (3378 files)
└── README.md             # Project documentation
```

### Architecture Overview

#### Key Components
- **Main Entry:** index.js, index.js, index.js, index.js, index.js
- **Test Suite:** 3378 test files
- **Build Configuration:** package.json, package.json, Makefile

#### Design Principles

1. **Modularity** - Code organized by functionality with clear separation of concerns
2. **Testability** - Comprehensive test coverage across critical paths
3. **Clarity** - Explicit naming and structure for AI agent understanding
4. **Consistency** - Uniform patterns and conventions throughout codebase
5. **Maintainability** - Well-documented code with clear intent

### Directory Map

| Directory | Purpose |
|-----------|----------|
| `docs/` | Documentation |
| `src/` | Source code |


### Development Workflow

#### Initial Setup

```bash
git clone https://github.com/YOUR_ORG/service-fabric.git
cd service-fabric
pip install -e .
# or
uv sync --all-groups
```

#### Development Commands

**Running Tests:**
```bash
pytest                    # Run all tests
pytest tests/             # Run specific test directory
pytest -v                 # Verbose output with test names
pytest -x                 # Stop on first failure
coverage run -m pytest && coverage report  # With coverage report
```

#### Code Quality
```bash
ruff check .              # Lint with ruff
ruff format .             # Format code
mypy .                    # Type checking (if configured)
```

### Code Style & Conventions

- **Naming:** Use C conventions (snake_case for functions, PascalCase for classes)
- **Type Hints:** Yes (strongly encouraged)
- **Error Handling:** Yes - handle errors at boundaries; let exceptions propagate when another layer owns recovery
- **Logging:** Yes
- **Testing:** Yes - write tests alongside code changes

### Testing Strategy

**Framework:** None detected
**Test Files:** 3378 found

Before committing:
1. Run the full test suite: `pytest`
2. Ensure all tests pass
3. Check type hints: `mypy .`
4. Format code: `ruff format .`

### Writing Documentation

When updating docs:
1. Always include explanatory text before code snippets
2. Describe *why* and *what* before showing *how*
3. Keep sections focused on a single concept
4. Use clear, concrete examples

## Known Gotchas & Warnings

- For any work related to setting up build, test, and CI for Service Fabric on GitHub, or for small patches or bug fixes, please open an issue for tracking purposes, but we generally don't need a discussion prior to opening a PR.

### Contributing Guidelines

This project has a detailed contribution guide at **`CONTRIBUTING.md`**.

**Key Requirements:**
- Review the contribution guide for all requirements
- Follow established patterns in the codebase
- Ensure alignment with project's contribution policies

### Common Patterns

When contributing to this project:
1. Read existing code in the area you're modifying
2. Follow the established patterns and style
3. Write tests for new functionality
4. Use clear, descriptive variable and function names
5. Add docstrings for public APIs
6. Update tests when changing behavior

### What We Value

✅ Well-tested code with clear intent
✅ Consistent code style and naming conventions
✅ Code that is easy for AI agents to understand
✅ Clear, descriptive commit messages
✅ Modular, reusable components
✅ Comprehensive documentation

### What We Avoid

❌ Large functions doing multiple things
❌ Commented-out dead code
❌ Inconsistent naming or patterns
❌ Unclear error messages
❌ Unexplained magic numbers or strings
❌ Skipped tests or test TODOs

### AI Readiness Dimensions (Scoring)

This project is evaluated across 8 dimensions:

1. **Architecture** (20/100) - Code organization and modularity
2. **Testing** (15/100) - Test coverage and quality
3. **Dependencies** (12/100) - Dependency management
4. **Conventions** (8/100) - Consistent patterns
5. **Entry Points** (10/100) - Clear main/start locations
6. **Security** (10/100) - Input validation and error handling
7. **Build** (10/100) - Clear build/setup instructions
8. **Documentation** (8/100) - Code and project documentation

### Next Steps

Before making changes:
1. Read relevant source files to understand the existing code
2. Look at existing tests for similar functionality
3. Follow the patterns you see in the codebase
4. Write tests for your changes
5. Run `pytest` to verify nothing breaks
6. Run code quality checks: `ruff check . && mypy .`
7. Format your code: `ruff format .`

---

*Generated by Braxis - keeping AI agents in sync with your code*
