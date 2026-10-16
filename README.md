# ShuttleOpt — Static Shuttle Schedule Optimizer for Urban Transit

ShuttleOpt reads GPS stop coordinates and arrival deadlines from a CSV file, calculates inter-stop travel times using the **Haversine formula** and **Floyd-Warshall** all-pairs shortest paths, then builds a shuttle schedule in two stages:

1. **Greedy baseline** — assigns stops to the shuttle that can arrive earliest, sorted by deadline.
2. **Local-search optimizer** — improves the schedule using 2-opt (route reversal) and inter-shuttle stop relocation.

Final results are printed to the terminal and saved to `output/final_schedule.csv`.

---

## How to Compile

```bash
# Using Makefile (recommended)
make

# Or manually
g++ -Iinclude -Wall -std=c++14 src/main.cpp src/data.cpp src/scheduler.cpp src/optimizer.cpp src/evaluator.cpp -o shuttle_optimizer
```

## How to Run

```bash
# Default (Dehradun 4-stop dataset, 2 shuttles)
.\shuttle_optimizer.exe

# Custom file and shuttle count
.\shuttle_optimizer.exe data\stops.csv 2

# Edge-case test (non-contiguous stop IDs)
.\shuttle_optimizer.exe data\stops_edge.csv 2
```

---

## Project Structure

```
shuttle-optimizer/
├── include/shuttle.h       # Domain models + function declarations
├── src/
│   ├── main.cpp            # CLI entry point, pipeline orchestration
│   ├── data.cpp            # CSV loader, Haversine + Floyd-Warshall matrix
│   ├── scheduler.cpp       # Greedy baseline scheduler
│   ├── optimizer.cpp       # Local-search optimiser (2-opt + relocate)
│   └── evaluator.cpp       # Schedule printer and CSV exporter
├── data/
│   ├── stops.csv           # 4 Dehradun stops (IDs 0-3)
│   └── stops_edge.csv      # Edge-case: non-contiguous stop IDs
├── output/                 # Generated schedule CSV (auto-created)
├── docs/                   # Detailed documentation
├── Makefile
└── PROJECT_CONTEXT.md      # Quick reference for this codebase
```

---

## DSA Concepts Used

- **Adjacency matrix** — `N×N vector<vector<int>>` for travel times
- **Floyd-Warshall** — all-pairs shortest path in O(n³)
- **Greedy algorithm** — earliest-deadline-first stop assignment
- **2-opt local search** — intra-route sub-sequence reversal
- **Relocation** — inter-shuttle stop migration
- **STL sort** with `operator<` overloading

## OOP Concepts Used

- 4 classes: `Config`, `Stop`, `ScheduleEntry`, `ShuttleSchedule`
- Constructors (default + parameterized), destructor
- `operator<` and `operator<<` overloading
- `friend` functions, `static` members
- Modular design with shared header contract

---

## Assumptions

- Shuttle speed: 25 km/h (reasonable for Dehradun city traffic)
- Dwell time per stop: 2 minutes
- All shuttles start at 07:00 AM
- Coordinates are approximate real Dehradun locations

---

## Documentation

See `docs/` folder:
- `PROJECT_WALKTHROUGH.md` — how the code works, explained for beginners
- `BUILD_FROM_SCRATCH.md` — how to recreate this project from an empty folder
- `PBL_PRESENTATION_GUIDE.md` — presentation script, demo steps, 20 viva Q&A
- `FUTURE_SCOPE.md` — roadmap for future improvements