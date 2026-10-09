# CricPulse — Live Cricket Analytics & Match Center Dashboard

CricPulse is an interactive cricket analytics hub and match center dashboard built for cricket analysts, coaching staff, and fans. It allows users to track live ball-by-ball match feeds, analyze over-by-over innings momentum, explore player partnership networks, manage private tactical workspaces, and participate in interactive fan polls powered by an in-memory C++17 service.

---

## 1. Application Overview

### Core Functionality
- **Live Match & Ball-by-Ball Feed**: Real-time match scoreboard tracking runs, wickets, current run rate (CRR), required run rate (RRR), win probability, and chronological event logs.
- **Innings Momentum & Over Breakdown**: Interactive over-by-over scoring distribution chart highlighting top scoring overs and continuous run stretches.
- **Rolling Run Rate Tracking**: Calculates dynamic scoring pace over the most recent 3 overs with differential benchmarking against the overall innings average.
- **Partnership Network & Bottleneck Analysis**: Undirected graph modeling player batting connections with maximum-bottleneck capacity path calculations between batting pairs.
- **Player Workspace & Access Control**: Role-restricted tactical focus notes protected by role-based access validation.
- **Fan Zone & Interactive Polls**: Real-time polling with per-user rate limiting and live vote share percentage breakdowns.

### Technology Stack
- **Language / Standard**: C++17
- **HTTP & JSON Libraries**: Single-header `httplib.h` (`cpp-httplib`) and `nlohmann/json.hpp`
- **Frontend**: Vanilla JavaScript (ES6+), HTML5, Vanilla CSS (CSS custom properties, glassmorphism, responsive layout, dark sports theme, SVG network graph)


## 2. Debugging Challenge

QA engineers and match analysts have flagged several issues in the CricPulse analytics platform. Your goal is to investigate the codebase, reproduce each bug, and implement the necessary fixes so that all automated test suites pass.

### Reported Issues & Tasks:

#### Issue 1: Partnership Reachability Misses Branching Connected Players
- **User Symptom**: When clicking on Rohit Sharma in the partnership network graph on the dashboard, a warning banner appears stating "Warning: Only 1 teammate is connected to Rohit Sharma", failing to explore branching batting partnerships across the squad.
- **Task**: Fix the recursive traversal in `src/partnership_reachability.cpp` so that it visits all reachable teammates across sibling branches instead of returning prematurely on the first neighbor.

#### Issue 2: Best Six-Over Stretch Calculation Skips Overlapping Windows
- **User Symptom**: The innings momentum card displays 86 runs for the best six-over stretch instead of the true peak stretch of 108 runs (overs 3–8), because the sliding window skips by full 6-over chunks.
- **Task**: Correct the sliding window loop in `src/best_six_over.cpp` to advance one over at a time (`start += 1`), evaluating all overlapping six-over periods.

#### Issue 3: Rolling Run Rate Computes Innings Average Instead of Recent Form
- **User Symptom**: The Live Form card displays a rolling run rate of 13.42 runs/over (the overall innings average) instead of the recent pace of 11.33 runs/over across the last 3 overs.
- **Task**: Update the rolling run rate calculation in `src/rolling_rate.cpp` to accurately compute scoring rate exclusively across the most recent three completed overs.

#### Issue 4: Strongest Partnership Chain Chooses Fewest Hops Instead of Maximizing Bottleneck
- **User Symptom**: When computing the strongest partnership chain between Rohit and Jadeja, the platform suggests the direct route Rohit ➔ Kohli ➔ Jadeja with a 20-run bottleneck, ignoring the stronger chain Rohit ➔ Gill ➔ Pandya ➔ Jadeja with a 30-run bottleneck.
- **Task**: Refactor the search algorithm in `src/strongest_chain.cpp` to find the path that maximizes the minimum link capacity (bottleneck) between two players rather than minimizing unweighted hops.

#### Issue 5: Fan Accounts Permitted to Edit Player Focus Notes
- **User Symptom**: Users logged in with unprivileged fan accounts (`role == "fan"`) are able to edit and save private tactical focus notes in the Player Workspace, which should be restricted strictly to players.
- **Task**: Enforce strict role-based access control in `src/player_access.cpp` so that only accounts with `role == "player"` can modify player tactical notes, returning HTTP 403 for unauthorized roles.

#### Issue 6: Fan Poll Rate Limiting is Shared Globally Rather Than Per User
- **User Symptom**: When a fan user exhausts their voting allowance in the Fan Zone poll, all other users are immediately blocked with HTTP 429 "Poll limit reached", because rate limiting is tracked globally rather than independently per user.
- **Task**: Update `src/poll_limiter.cpp` to track rate-limiting windows per individual fan (`userId`) so that one user's activity does not exhaust the quota for other fans.

---

## 3. Expected Behavior After Fixing Bugs

After resolving the issues:
1. Clicking any player in the partnership network graph identifies all connected teammates across all graph branches.
2. The best six-over calculation evaluates all overlapping windows and accurately reports 108 runs.
3. The rolling run rate correctly reflects scoring pace across the last 3 overs (11.33 runs/over).
4. The strongest partnership chain identifies the path maximizing bottleneck capacity (30 runs between Rohit and Jadeja).
5. Only authenticated player accounts can edit player focus notes; fan accounts receive an authorization error.
6. Poll voting limits are enforced independently per user, allowing separate fans to cast their full quota of votes.
7. All automated tests in `tests/` pass with exit code `0`.
