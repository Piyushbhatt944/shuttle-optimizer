# ShuttleOpt — Static Shuttle Schedule Optimizer for Urban Transit

ShuttleOpt reads GPS stop coordinates and arrival deadlines from a CSV file, calculates inter-stop travel times using the **Haversine formula** and **Floyd-Warshall** all-pairs shortest paths, then builds a shuttle schedule in two stages:

1. **Greedy baseline** — assigns stops to the shuttle that can arrive earliest, sorted by deadline.
2. **Local-search optimizer** — improves the schedule using 2-opt (route reversal) and inter-shuttle stop relocation.

Final results are printed to the terminal, saved to `output/final_schedule.csv`, and can be visualized on an interactive map dashboard.

---

## How to Compile

```bash
make
# Or manually:
g++ -Iinclude -Wall -std=c++14 src/main.cpp src/data.cpp src/scheduler.cpp src/optimizer.cpp src/evaluator.cpp -o shuttle_optimizer
```

## How to Run

```bash
# Default (Dehradun 4-stop dataset, 2 shuttles, 25 km/h, 07:00 start)
.\shuttle_optimizer.exe

# Custom file and shuttle count
.\shuttle_optimizer.exe data\stops.csv 2

# With custom parameters
.\shuttle_optimizer.exe data\stops.csv 2 --speed 15 --start 06:30 --dwell 3

# Show all options
.\shuttle_optimizer.exe --help
```

### CLI Options

| Flag | Description | Default |
|---|---|---|
| `--speed <km/h>` | Average shuttle speed | 25.0 |
| `--start <HH:MM>` | Fleet departure time | 07:00 |
| `--dwell <mins>` | Boarding time per stop | 2 |
| `--help` | Show usage | — |

## Web Dashboard

Open `web/index.html` in any browser and load `output/final_schedule.csv` to see:
- Interactive Dehradun map with color-coded shuttle routes
- KPI cards (stops served, shuttles used, time range)
- Formatted schedule table

No server or installation needed — just open the HTML file.

---

## Project Structure

```
shuttle-optimizer/
├── include/shuttle.h       # Domain models + function declarations
├── src/
│   ├── main.cpp            # CLI entry point with flag parsing
│   ├── data.cpp            # CSV loader, Haversine + Floyd-Warshall matrix
│   ├── scheduler.cpp       # Greedy baseline scheduler
│   ├── optimizer.cpp       # Local-search optimiser (2-opt + relocate)
│   └── evaluator.cpp       # Schedule printer and CSV exporter
├── data/
│   ├── stops.csv           # 4 Dehradun stops (IDs 0-3)
│   └── stops_edge.csv      # Edge-case: non-contiguous stop IDs
├── web/
│   └── index.html          # Single-file map dashboard (Leaflet.js)
├── output/                 # Generated schedule CSV (auto-created)
├── Makefile
└── README.md
```

---

## DSA Concepts Used

- Adjacency matrix — N×N vector for travel times
- Floyd-Warshall — all-pairs shortest path in O(n³)
- Greedy algorithm — earliest-deadline-first stop assignment
- 2-opt local search — intra-route sub-sequence reversal
- Relocation — inter-shuttle stop migration
- STL sort with operator< overloading

## OOP Concepts Used

- 4 classes: Config, Stop, ScheduleEntry, ShuttleSchedule
- Constructors (default + parameterized), destructor
- operator< and operator<< overloading
- Friend functions, static members
- Modular design with shared header contract

---

## Assumptions

- Shuttle speed: configurable (default 25 km/h)
- Dwell time per stop: configurable (default 2 minutes)
- Start time: configurable (default 07:00 AM)
- Coordinates are approximate real Dehradun locations
