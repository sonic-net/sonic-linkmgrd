# Copilot Instructions for sonic-linkmgrd

## Project Overview

sonic-linkmgrd (Link Manager Daemon) manages link switching decisions in SONiC dual-ToR (Top of Rack) deployments. In a dual-ToR setup, each server is connected to two ToR switches via Y-cables. linkmgrd determines which ToR is active for each port, handles link failure detection, and orchestrates switchover between the active and standby ToR switches.

## Architecture

```
sonic-linkmgrd/
├── src/                   # C++ source code
│   ├── link_manager/      # Core link management logic
│   ├── link_prober/       # Link probing/health check
│   ├── mux_manager/       # MUX state management
│   └── common/            # Shared utilities
├── test/                  # Unit tests
├── doc/                   # Design documentation
├── debian/                # Debian packaging
├── Makefile               # Build entry point
├── CODE_OF_CONDUCT.md
├── SECURITY.md
├── SUPPORT.md
└── .github/               # GitHub configuration
```

### Key Concepts
- **Dual-ToR**: Two ToR switches connected to same servers via Y-cables
- **Active/Standby**: One ToR is active (forwarding traffic), other is standby
- **Link probing**: Periodic health checks to detect link failures
- **MUX cable**: Y-cable hardware that switches between ToR connections
- **Switchover**: Process of moving active role from one ToR to another

## Language & Style

- **Primary language**: C++
- **C++ standard**: C++14/17
- **Indentation**: 4 spaces
- **Naming conventions**:
  - Classes: `PascalCase`
  - Methods: `camelCase`
  - Variables: `camelCase`, `m_` prefix for members
  - Constants: `UPPER_CASE`
  - Files: `PascalCase` or `snake_case`
- **Build system**: Make + autotools

## Build Instructions

```bash
# Install dependencies (libswsscommon, boost, etc.)

# Build
make

# Build Debian package
dpkg-buildpackage -us -uc -b
```

## Testing

```bash
# Run unit tests
cd test
make
./test_linkmgrd
```

- Tests use **Google Test** framework
- Tests verify state machine transitions, link probing logic, and switchover scenarios
- Integration tests in sonic-mgmt repo (dual-ToR topology)

## PR Guidelines

- **Commit format**: `[component]: Description`
- **Signed-off-by**: REQUIRED (`git commit -s`)
- **CLA**: Sign Linux Foundation EasyCLA
- **State machine**: Document state transition changes
- **Testing**: Add unit tests for new state transitions or link management logic

## Common Patterns

### State Machine Pattern
```cpp
// linkmgrd uses a state machine for each MUX port
// States: Active, Standby, Unknown, Wait
// Events: LinkUp, LinkDown, ProbeSuccess, ProbeFail
// Transitions trigger MUX cable switching
```

### Link Probing
```cpp
// Periodic ICMP/custom probes sent through each link
// Probe results feed into the state machine
// Configurable intervals and thresholds
```

## Dependencies

- **sonic-swss-common**: Database connectivity
- **Boost**: C++ utility libraries
- **libnl**: Netlink communication
- **Google Test**: Testing framework

## Gotchas

- **State machine complexity**: The dual-ToR state machine has many edge cases — test thoroughly
- **Race conditions**: Link events can arrive in any order — state machine must handle this
- **Timer management**: Probe timers and switchover timeouts must be precise
- **Y-cable hardware**: Different Y-cable vendors may behave differently
- **Traffic loss**: Incorrect switchover logic causes traffic blackholing
- **Warm restart**: State must be preserved during warm restart
- **Multi-thread**: Be careful with thread safety in probe and state management code
