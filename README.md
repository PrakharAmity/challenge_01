# CricPulse — Live Cricket Analytics Platform

CricPulse is a high-performance live cricket analytics hub and match center dashboard built for cricket analysts, coaching staff, and fans. It allows users to track live ball-by-ball match feeds, analyze over-by-over innings momentum, explore player partnership routes, manage authenticated team sessions, and participate in interactive fan polls powered by an in-memory C++20 service.

---

## 1. Application Overview

### Core Modules & Analytics
- **Live Match & Ball-by-Ball Feed**: Scoreboard tracking innings runs, wickets, legal balls, current run rate, and chronological delivery commentary.
- **Innings Momentum & Best Six-Over Stretch (`src/analytics.cpp`)**: Evaluates continuous six-over sliding windows across completed overs to identify the highest-scoring phases of play.
- **Current Run Rate (`src/run_rate.cpp`)**: Computes current scoring rate per over, accounting for full and partial overs with floating-point precision.
- **Player Authentication (`src/auth.cpp`)**: Manages player workspace sessions using stateless HMAC signatures and time-based expiration thresholds.
- **Fan Zone Rate Limiting (`src/rate_limiter.cpp`)**: Enforces per-fan polling cooldown intervals to maintain fair community engagement.
- **Partnership Network Routes (`src/partnership_routes.cpp`)**: Evaluates optimal connection paths between batting partners using Dijkstra's shortest-path algorithm over accumulated partnership fatigue weights.
- **Recursive Partnership Chains (`src/partnership_chain.cpp`)**: Explores recursive depth-first partnership paths across the team network to determine maximum partnership chains.

### Technology Stack
- **Language / Standard**: C++20
- **HTTP & JSON Libraries**: Single-header `httplib.h` and `nlohmann/json.hpp`
- **Frontend**: Vanilla JavaScript (ES6+), HTML5, Vanilla CSS (CSS custom properties, responsive grid, inline SVG charts and graphs)

---

## 2. Getting Started

### Building the Project
Configure and compile the application with CMake:
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
```

### Running the Server
Launch the application watcher:
```bash
./start.sh
```
The server binds to `0.0.0.0:8080`. Frontend assets in `web/` reload dynamically, while C++ edits trigger an automated background rebuild and reload.

### Running Automated Verification
Run the verification test suite:
```bash
./tests/run_tests.sh
```
Results are output as strict JSON telemetry indicating test status and execution time.

---

## 3. Architecture & Source Layout

```text
cricpulse-cpp-challenge/
├── CMakeLists.txt              Build configuration (C++20, targets: server, challenge_tests)
├── challenge.json              Runtime environment and test execution configuration
├── README.md                   Project overview and documentation
├── start.sh                    Application launcher and file watcher
├── include/
│   ├── analytics.hpp           Sliding-window continuous stretch declarations
│   ├── run_rate.hpp            Current run-rate declarations
│   ├── auth.hpp                Player session validation and HMAC signature declarations
│   ├── rate_limiter.hpp        Fan poll cooldown limiter declarations
│   ├── partnership_routes.hpp  Partnership routing declarations
│   ├── partnership_chain.hpp   Recursive partnership chain declarations
│   ├── match_data.hpp          Domain structures and sample fixture declarations
│   ├── httplib.h               Single-header HTTP server
│   └── nlohmann/
│       └── json.hpp            Single-header JSON library
├── src/
│   ├── analytics.cpp           Sliding-window continuous stretch calculation
│   ├── run_rate.cpp            Current run-rate calculation
│   ├── auth.cpp                Session expiry validation and token generation
│   ├── rate_limiter.cpp        Poll cooldown tracking and attempt evaluation
│   ├── partnership_routes.cpp  Dijkstra partnership route calculation
│   ├── partnership_chain.cpp   Recursive partnership chain search
│   ├── match_data.cpp          Sample match fixtures and graph definitions
│   └── main.cpp                HTTP service, REST endpoints, and static asset delivery
├── web/
│   ├── index.html              Dashboard layout and card structures
│   ├── style.css               Dark-theme sports analytics styling and animations
│   └── app.js                  Interactive client logic and SVG visualization
└── tests/
    ├── challenge_tests.cpp     Automated test suite with JSON telemetry
    └── run_tests.sh            Test execution script
```
