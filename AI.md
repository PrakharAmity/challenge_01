# CricPulse — Live Cricket Analytics (C++17 Edition)

## Description

CricPulse is a C++17 live cricket analytics hub with a ball-by-ball feed, innings momentum panels, partnership network graph, player focus-note access, and fan polls. It runs as a lightweight HTTP service on port 3000 (`0.0.0.0:3000`) with an interactive browser interface and an in-memory match fixture catalog. The analytics engine models live match statistics including over-by-over runs, partnership connections between batting pairs, rolling run rates, and role-based access control for players and fans.

## Repository Structure

```text
cricpulse-cpp-challenge/
├── CMakeLists.txt              Build configuration for core library, server, and tests (C++17)
├── challenge.json              Runtime environment, port 3000, build, start, and test command
├── README.md                   Project overview and developer documentation
├── AI.md                       Detailed challenge and bug documentation
├── include/
│   ├── match_data.hpp          Match state structures and sample match declarations
│   ├── best_six_over.hpp       Best six-over continuous stretch declaration
│   ├── rolling_rate.hpp        Rolling run rate declaration
│   ├── strongest_chain.hpp     Strongest partnership chain declaration
│   ├── partnership_reachability.hpp Recursive graph traversal declaration
│   ├── player_access.hpp       Role-based note permission declarations
│   ├── poll_limiter.hpp        Sliding-window fan poll limiter declaration
│   ├── httplib.h               Single-header cpp-httplib HTTP server
│   └── nlohmann/
│       └── json.hpp            Single-header nlohmann JSON library
├── src/
│   ├── main.cpp                HTTP server, REST endpoints, auth, and static file serving
│   ├── cric.cpp                Deterministic sample match fixture and JSON serialization
│   ├── best_six_over.cpp       Best six-over stretch implementation (Bug 2)
│   ├── rolling_rate.cpp        Rolling run rate implementation (Bug 3)
│   ├── strongest_chain.cpp     Partnership chain search implementation (Bug 4)
│   ├── partnership_reachability.cpp Recursive graph traversal implementation (Bug 1)
│   ├── player_access.cpp       Role access validation implementation (Bug 5)
│   └── poll_limiter.cpp        Fan poll rate limiting implementation (Bug 6)
├── web/
│   ├── index.html              CricPulse live match center layout and cards
│   ├── style.css               Dark-theme sports analytics CSS with responsive styling
│   └── app.js                  Interactive dashboard client, REST API sync, and SVG graph
└── tests/
    ├── run_tests.cpp           Automated runner with strict single-line JSON telemetry
    └── run_tests.sh            Incremental test build and test runner execution script
```

---

## Bugs and Bug Locations

These are the six behavioral bug surfaces covered by the challenge. All six bugs are intentionally present initially and can be observed both through the test suite and live in the browser dashboard.

### 1. Partnership Reachability Misses Branching Connected Players
- **Bug Location:** `src/partnership_reachability.cpp`, helper `visit`
- **Test Name:** `test_recursive_partnership_scan_visits_all_connected_players`
- **How to Observe:**
  - *CLI/Test:* Run `./tests/run_tests.sh`. Test fails with `"Partnership scan missed a player on a second branch"`.
  - *Frontend:* Click Rohit Sharma's node in the partnership network graph on `http://localhost:3000`. A warning banner is displayed: `"Warning: Only 1 teammate is connected to Rohit Sharma"`.
- **Failure:** The helper function `visit` executes `return visit(state, link.player, seen, order);` inside the partner iteration loop on the first unvisited neighbor, immediately terminating sibling branch exploration and only finding 1 teammate instead of all connected teammates.
- **Expected:** The function should continue exploring all sibling links in the loop:
  ```cpp
  seen[link.player] = true;
  order.push_back(link.player);
  visit(state, link.player, seen, order);
  ```

### 2. Best Six-Over Stretch Calculation Skips Overlapping Windows
- **Bug Location:** `src/best_six_over.cpp`, `cricpulse::bestSixOverRuns`
- **Test Name:** `test_best_six_over_stretch_includes_overlapping_windows`
- **How to Observe:**
  - *CLI/Test:* Run `./tests/run_tests.sh`. Test fails with `"Expected best six-over stretch of 108 runs, got 86"`.
  - *Frontend:* Inspect the `"BEST 6 OVERS"` badge on the Innings Momentum card. It displays `86 RUNS` instead of `108 RUNS`.
- **Failure:** The sliding window loop increments by `start += 6` instead of `start += 1`, only evaluating discrete 6-over blocks (overs 1–6 = 86 runs, overs 7–12 = 75 runs) and missing overlapping windows like overs 3–8 (108 runs).
- **Expected:** The loop slides by one over at a time (`start++` or `start += 1`) across all available overs to evaluate all overlapping windows and find the maximum stretch (108 runs).

### 3. Rolling Run Rate Computes Innings Average Instead of Recent Form
- **Bug Location:** `src/rolling_rate.cpp`, `cricpulse::rollingRunRate`
- **Test Name:** `test_rolling_run_rate_uses_recent_overs`
- **How to Observe:**
  - *CLI/Test:* Run `./tests/run_tests.sh`. Test fails with `"Expected recent three-over rate near 11.33, got 13.416667"`.
  - *Frontend:* Inspect the Live Form card. The rolling run rate displays `13.42 / over` instead of `11.33 / over`.
- **Failure:** The function sums all runs across the entire innings and divides by total overs count (`inningsRuns / state.overs.size()`), calculating the overall innings average run rate rather than the recent 3-over rate.
- **Expected:** The function computes the run rate specifically over the last 3 completed overs (`(5 + 5 + 24) / 3 = 11.333333`).

### 4. Strongest Partnership Chain Chooses Fewest Hops Instead of Maximizing Bottleneck
- **Bug Location:** `src/strongest_chain.cpp`, `cricpulse::strongestPartnershipChain`
- **Test Name:** `test_partnership_chain_maximizes_minimum_link`
- **How to Observe:**
  - *CLI/Test:* Run `./tests/run_tests.sh`. Test fails with `"Expected strongest chain bottleneck of 30 runs, got 20"`.
  - *Frontend:* The Strongest Chain badge displays `Rohit ➔ Kohli ➔ Jadeja (20 runs bottleneck)` instead of 30 runs.
- **Failure:** The function uses standard unweighted Breadth-First Search (BFS) using `std::queue`. BFS finds the path with the fewest hops (`0 ➔ 1 ➔ 5`, bottleneck 20 runs) instead of the path maximizing minimum link capacity (`0 ➔ 2 ➔ 4 ➔ 5`, bottleneck 30 runs).
- **Expected:** The function finds the maximum-bottleneck path (e.g., using a max-bottleneck priority queue / modified Dijkstra or capacity path search) that maximizes the weakest link in the chain (bottleneck 30 runs).

### 5. Fan Accounts Permitted to Edit Player Focus Notes
- **Bug Location:** `src/player_access.cpp`, `cricpulse::canSavePlayerNote`; surfaced by `POST /api/player-note`
- **Test Name:** `test_fan_cannot_edit_player_focus_note`
- **How to Observe:**
  - *CLI/Test:* Run `./tests/run_tests.sh`. Test fails with `"Fan role must not edit a player-only focus note"`.
  - *Frontend:* Sign in as Fan 1 (`fan` / `fanpass`). In the Player Workspace card, click `"Save note"`. The note is successfully saved (HTTP 200).
- **Failure:** `canSavePlayerNote` returns `role == "player" || role == "fan"`, erroneously allowing unprivileged fan users to update private player tactical notes.
- **Expected:** The function must restrict permissions strictly to `role == "player"`. Fan accounts must be rejected with HTTP 403 Forbidden.

### 6. Fan Poll Rate Limiting is Shared Globally Rather Than Per User
- **Bug Location:** `src/poll_limiter.cpp`, `cricpulse::allowFanPoll`; surfaced by `POST /api/poll`
- **Test Name:** `test_poll_rate_limit_is_per_fan`
- **How to Observe:**
  - *CLI/Test:* Run `./tests/run_tests.sh`. Test fails with `"A second fan should have an independent poll allowance"`.
  - *Frontend:* Sign in as Fan 1 (`fan` / `fanpass`) and cast 3 votes in the Fan Zone poll until reaching the limit. Sign out and sign in as Fan 2 (`fan2` / `fanpass`). Casting a vote as Fan 2 is immediately blocked with HTTP 429 `"Poll limit reached"`.
- **Failure:** The function ignores `userId` via `(void)userId;` and uses a single static window counter for all incoming requests, causing any fan to exhaust the quota for everyone.
- **Expected:** Rate limiting must be maintained independently per fan (`userId`), allowing each fan account to have their own allowance within the time window.

---

## Strict JSON Test Telemetry Output

The test executable outputs ONLY a single-line strict JSON string to stdout:

```json
{"test_recursive_partnership_scan_visits_all_connected_players":{"Status":"failed","Execution time":"0ms","Error":"Partnership scan missed a player on a second branch"},"test_best_six_over_stretch_includes_overlapping_windows":{"Status":"failed","Execution time":"0ms","Error":"Expected best six-over stretch of 108 runs, got 86"},"test_rolling_run_rate_uses_recent_overs":{"Status":"failed","Execution time":"0ms","Error":"Expected recent three-over rate near 11.33, got 13.416667"},"test_partnership_chain_maximizes_minimum_link":{"Status":"failed","Execution time":"0ms","Error":"Expected strongest chain bottleneck of 30 runs, got 20"},"test_fan_cannot_edit_player_focus_note":{"Status":"failed","Execution time":"0ms","Error":"Fan role must not edit a player-only focus note"},"test_poll_rate_limit_is_per_fan":{"Status":"failed","Execution time":"0ms","Error":"A second fan should have an independent poll allowance"},"Passed":0,"Failed":6,"Total bugs":6,"Total Execution time":"0ms"}
```

- When any test fails, the process exits with status code 1.
- When all 6 tests pass, the process exits with status code 0.
