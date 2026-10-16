# ShuttleOpt — Project Context

## Purpose
ShuttleOpt is a C++14 CLI program that schedules urban transit shuttles across GPS stops. It reads stop data (coordinates, arrival deadlines) from a CSV file, computes inter-stop travel times using the Haversine formula and Floyd-Warshall algorithm, allocates stops to shuttles via a greedy algorithm, then improves the schedule using a local-search optimizer. Results are printed to the terminal and exported as a CSV file.

**Domain:** Dehradun city transit, 4 stops, 2 shuttles (configurable via CLI).

---

## Important Files

| File | Purpose |
|---|---|
| `include/shuttle.h` | Domain models (Config, Stop, ScheduleEntry, ShuttleSchedule), function declarations, `g_allStops` and `g_stopIdxMap` externs |
| `src/data.cpp` | CSV parsing (`loadStops`), Haversine distance, Floyd-Warshall matrix (`buildTravelTimeMatrix`), defines both globals |
| `src/scheduler.cpp` | Greedy scheduler (`generateBaselineSchedule`) — sorts stops by deadline, assigns to earliest-available shuttle |
| `src/optimizer.cpp` | Local-search optimizer (`optimizeSchedule`) — Phase 1: 2-opt, Phase 2: inter-shuttle stop relocation |
| `src/evaluator.cpp` | Schedule printer (`evaluateSchedule`) and CSV exporter (`exportScheduleToCSV`) |
| `src/main.cpp` | CLI entry point, drives the full pipeline |
| `data/stops.csv` | 4 Dehradun stops with GPS coords and deadlines (minutes since midnight) |
| `data/stops_edge.csv` | Edge-case test: same stops with non-contiguous IDs (7, 10, 25, 42) |
| `Makefile` | Build automation |

---

## Build and Run

**Compiler:** g++ (MinGW GCC 6.3.0+), C++14

```bash
# Build
make
# or manually:
g++ -Iinclude -Wall -std=c++14 src/main.cpp src/data.cpp src/scheduler.cpp src/optimizer.cpp src/evaluator.cpp -o shuttle_optimizer

# Run (default)
.\shuttle_optimizer.exe data\stops.csv 2

# Run (edge-case test)
.\shuttle_optimizer.exe data\stops_edge.csv 2
```

**Output:** Terminal table with arrival/departure/deadline/status per stop, metrics, and `output/final_schedule.csv`.

---

## Key Implementation Details

- **`Config`** stores all magic numbers: `START_TIME=420` (7:00 AM in minutes), `DWELL_TIME=2`, `MAX_ITERATIONS=500`, speed=25 km/h.
- **`g_stopIdxMap`** (`map<int,int>` in `data.cpp`) maps stop ID → 0-based matrix index. **Critical:** all matrix accesses in scheduler.cpp and optimizer.cpp must use `g_stopIdxMap.at(stopId)` — never use raw stop ID as index.
- **Travel time matrix** is an `N×N` `vector<vector<int>>` in minutes, smoothed by Floyd-Warshall.
- **Greedy phase**: sorts stops by `requiredByTime`, assigns each to the shuttle that arrives earliest.
- **Optimizer phases**: (1) 2-opt reverses sub-routes within a shuttle; (2) relocate moves a stop from one shuttle to another. Both check deadline feasibility before accepting.
- **Cost function**: `totalTravel + totalWait` (travel time + earliness slack). Lower = better.
- **Directory creation** uses `_mkdir` (Windows) / `mkdir` (Linux) — not `system()`.

---

## Current Status

**Working and tested:**
- ✅ Compiles with 0 errors, 0 warnings
- ✅ Normal run: 4/4 stops ON TIME, cost reduced 146 → 116 in 8 iterations
- ✅ Edge case (non-contiguous IDs): correct output, no crash
- ✅ Error cases: missing file and 0-shuttle count both exit with code 1 + descriptive message

**Remaining known limitations:**
- No passenger capacity or load constraints
- Speed and start time are compile-time constants (not configurable at runtime)
- Single-depot only (all shuttles start at same time from same implied origin)
- No automated test suite (`make test` not yet implemented)

---

## Remaining Tasks

1. Implement `make test` target in Makefile
2. Make speed and start time configurable via CLI flags
3. Add demand/capacity fields to Stop and ShuttleSchedule (see `docs/FUTURE_SCOPE.md`)
4. Optional: JSON export alternative in evaluator.cpp

---

## Documentation

| File | Contents |
|---|---|
| `docs/PROJECT_WALKTHROUGH.md` | Beginner-friendly guide to the entire codebase |
| `docs/BUILD_FROM_SCRATCH.md` | How to recreate this project from scratch |
| `docs/PBL_PRESENTATION_GUIDE.md` | Presentation script, demo steps, 20 viva Q&A, team division |
| `docs/FUTURE_SCOPE.md` | Short/medium/long-term improvement roadmap |
