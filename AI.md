# CricPulse — Live Cricket Analytics

## Description

CricPulse is a C++ live cricket analytics hub with a ball-by-ball feed, innings momentum panels, partnership network graph, player focus-note access, and fan polls. It runs as a lightweight HTTP service with a browser interface and an in-memory match fixture catalog. The analytics engine models live match statistics including over-by-over runs, partnership connections between batting pairs, rolling run rates, and role-based access control for players and fans.

## Repository Structure

```text
cricpulse-cpp-challenge/
├── CMakeLists.txt          Build configuration for core library, server, and tests
├── challenge.json          Isolated runtime, port, build, start, and test configuration
├── start.sh                Build, launch, and live-rebuild watcher entrypoint
├── include/
│   └── cric.hpp            Match state structures, data models, and analytics API declarations
├── src/
│   ├── best_six_over.cpp           Best six-over continuous stretch calculation
│   ├── cric.cpp                    Deterministic sample match fixture and JSON serialization
│   ├── main.cpp                    HTTP server, session management, REST routes, and static assets
│   ├── partnership_reachability.cpp Recursive graph traversal for connected batting partnerships
│   ├── player_access.cpp           Role-based permission check and player focus-note editing
│   ├── poll_limiter.cpp            Sliding-window rate-limiting for fan poll votes
│   ├── rolling_rate.cpp            Rolling run rate calculation across recent overs
│   └── strongest_chain.cpp         Maximum-bottleneck partnership chain search and chain strength
├── web/
│   ├── app.js               Live query handling, API calls, graph rendering, and UI updates
│   ├── index.html           CricPulse match center page structure and user-facing copy
│   └── style.css            Responsive sports analytics visual design
├── tests/
│   ├── run_tests.sh        Incremental test rebuild and JSON test runner script
│   └── challenge_tests.cpp Six behavioral challenge tests
└── README.md               Candidate-facing application overview and execution guide
```

## Bugs and Bug Locations

These are the six behavioral bug surfaces covered by the challenge. The named locations identify the owning implementation areas for debugging and review.

### 1. Recursive partnership scan misses branching connected players

- **Bug location:** `src/partnership_reachability.cpp`, `cricpulse::partnershipReachable` (and helper `visit`)
- **How to observe it:** Perform a partnership reachability traversal from a batsman whose partnerships branch across multiple batting partners (e.g., player 0 connects to players 1 and 2, and player 2 connects to player 3).
- **Failure:** The helper function `visit` executes an early `return` inside the partner iteration loop on the first unvisited link, terminating the scan before other branches or sibling links are traversed.
- **Expected:** The partnership scan traverses all branches recursively and discovers all connected players in the partnership network.

### 2. Best six-over stretch calculation skips overlapping sliding windows

- **Bug location:** `src/best_six_over.cpp`, `cricpulse::bestSixOverRuns`
- **How to observe it:** Inspect the "BEST 6 OVERS" badge on the innings momentum card or query `/api/analytics` for a match where peak scoring spans across non-multiple-of-6 intervals (e.g., overs 3 to 8).
- **Failure:** The loop increments the start index by 6 (`start += 6`) instead of 1, evaluating only disjoint blocks of overs and missing the true highest-scoring overlapping 6-over phase.
- **Expected:** The function slides a 6-over window by one over at a time (`start++`) across the entire innings to find the continuous 6-over window with the maximum total runs (108 runs in the sample match).

### 3. Rolling run rate computes entire innings average instead of recent overs

- **Bug location:** `src/rolling_rate.cpp`, `cricpulse::rollingRunRate`
- **How to observe it:** Check the "Rolling run rate" display on the live form card or query `/api/analytics`.
- **Failure:** The function divides total innings runs across all overs by total overs count (`inningsRuns / state.overs.size()`), yielding the cumulative match run rate (~13.42) rather than the recent 3-over rolling rate.
- **Expected:** The rolling run rate is calculated over the most recent 3 completed overs (e.g., overs 10, 11, and 12 yielding 34 runs over 3 overs ≈ 11.33 runs/over).

### 4. Strongest partnership chain minimizes hop count instead of maximizing bottleneck link strength

- **Bug location:** `src/strongest_chain.cpp`, `cricpulse::strongestPartnershipChain`
- **How to observe it:** Inspect the partnership chain path and bottleneck strength between player 0 (Rohit Sharma) and player 5 (Ravindra Jadeja) via `/api/analytics` or the player connection graph.
- **Failure:** The function uses standard unweighted breadth-first search (BFS), which selects the shortest path by hop count (`0 -> 1 -> 5`, bottleneck 20 runs) rather than the path with the strongest minimum partnership link (`0 -> 2 -> 4 -> 5`, bottleneck 30 runs).
- **Expected:** The path search uses a maximum-bottleneck path algorithm (e.g., modified Dijkstra or max-min priority queue) to find the partnership path between two players that maximizes the weakest link in the chain (bottleneck strength of 30 runs).

### 5. Fan role is permitted to edit player focus notes

- **Bug location:** `src/player_access.cpp`, `cricpulse::canSavePlayerNote`; surfaced by `src/main.cpp`, `POST /api/player-note`
- **How to observe it:** Sign in as a fan user (`fan` / `fanpass`) and attempt to submit an update to the player focus note.
- **Failure:** `canSavePlayerNote` allows `role == "fan"` alongside `role == "player"`, allowing unprivileged fans to modify private player tactical notes.
- **Expected:** Only authenticated users with the `player` role can save player focus notes; requests with the `fan` role are rejected with an access error (HTTP 403 Forbidden).

### 6. Fan poll rate limiting is shared globally rather than tracked per user

- **Bug location:** `src/poll_limiter.cpp`, `cricpulse::allowFanPoll`; surfaced by `src/main.cpp`, `POST /api/poll`
- **How to observe it:** Have one fan submit poll votes until the rate limit is reached, then have a second distinct fan submit a vote.
- **Failure:** The rate limiter ignores `userId` via `(void)userId;` and uses a single static window counter for all requests, exhausting the poll quota globally and rejecting votes from other users.
- **Expected:** Rate limiting is enforced independently per fan (`userId`), ensuring each user has their own poll allowance within the time window.

## Expected Behaviour After Fixing All Bugs

- The recursive partnership scan visits all connected players across all branches in the graph.
- The best six-over calculation evaluates all overlapping six-over windows and identifies the maximum run stretch (108 runs).
- The rolling run rate correctly reflects scoring momentum across the most recent 3 overs (~11.33 runs/over).
- The strongest partnership chain selects the path maximizing the bottleneck partnership link (strength of 30 runs).
- Fan users cannot edit player focus notes, restricting updates exclusively to player accounts.
- Fan poll voting rate limits are tracked per individual user rather than globally across all fans.
- All behavioral tests in `tests/challenge_tests.cpp` pass with exit code 0.
