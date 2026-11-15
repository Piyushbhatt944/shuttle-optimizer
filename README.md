# ShuttleOpt — Static Shuttle Schedule Optimizer for Urban Transit

ShuttleOpt is an algorithmic urban transit scheduling and route optimization engine developed in C++14. It takes real-world transit stop data (GPS latitude/longitude coordinates and arrival deadlines) from a CSV file, computes an all-pairs travel-time matrix using spatial trigonometry and shortest-path graph algorithms, generates a deadline-compliant baseline schedule, and refines it using local search metaheuristics.

The final schedule is printed to the terminal, saved to `output/final_schedule.csv`, and can be inspected on an auto-loading web dashboard.

---

## Table of Contents
1. [End-to-End Pipeline](#end-to-end-pipeline)
2. [What's Under the Hood (Core Algorithms)](#whats-under-the-hood-core-algorithms)
3. [OOP & DSA Concepts](#oop--dsa-concepts)
4. [Project Structure](#project-structure)
5. [Compilation & Build](#compilation--build)
6. [Execution & CLI Options](#execution--cli-options)
7. [Web Dashboard](#web-dashboard)

---

## End-to-End Pipeline

```
[stops.csv] (Input CSV with IDs, Names, Lat, Lon, Deadlines)
     │
     ▼  loadStops() + g_stopIdxMap (ID -> Matrix Index mapping)
vector<Stop> (In-memory C++ Stop objects)
     │
     ▼  buildTravelTimeMatrix() (Haversine Formula + Floyd-Warshall)
Matrix N×N (All-pairs shortest transit durations in minutes, scaled by --speed)
     │
     ▼  generateBaselineSchedule() (Greedy Earliest Deadline First)
Baseline Schedule (Stops assigned across shuttles starting at --start)
     │
     ▼  optimizeSchedule() (2-Opt Inversion + Inter-Shuttle Relocation)
Optimized Schedule (Minimizes travel time + slack while meeting all deadlines)
     │
     ├──► Terminal Table (Formatted arrival, departure, on-time status)
     └──► output/final_schedule.csv (Exported timetable)
               │
               ▼  Loaded into web/index.html
     [Stops Table + Schedule Timetable in Browser]
```

---

## What's Under the Hood (Core Algorithms)

### 1. Haversine Distance Formula (`src/data.cpp`)
Computes great-circle distance between two GPS coordinates over the curved surface of the Earth ($R = 6371\text{ km}$):
$$d = 2R \arcsin\left(\sqrt{\sin^2\left(\frac{\Delta \phi}{2}\right) + \cos(\phi_1)\cos(\phi_2)\sin^2\left(\frac{\Delta \lambda}{2}\right)}\right)$$
Travel duration between stops is derived using the configurable speed parameter (`--speed`, default 25 km/h):
$$\text{Time (minutes)} = \max\left(1, \text{round}\left(\frac{\text{Distance (km)}}{\text{Speed (km/h)}} \times 60\right)\right)$$

### 2. Floyd-Warshall Algorithm (`src/data.cpp`)
An all-pairs shortest path dynamic programming algorithm operating in $O(V^3)$ time complexity. It guarantees that the travel-time matrix satisfies the triangle inequality ($d[i][j] \le d[i][k] + d[k][j]$) across all transit hubs.

### 3. Safe Matrix Index Mapping (`g_stopIdxMap`)
A global lookup map (`std::map<int, int>`) decouples arbitrary or non-contiguous stop IDs (e.g., 7, 10, 25, 42) from contiguous matrix coordinates ($0 \dots N-1$). This prevents out-of-bounds memory indexing and segmentation faults.

### 4. Greedy Baseline Scheduler (`src/scheduler.cpp`)
Sorts stops chronologically by deadline using `Stop::operator<` (Earliest Deadline First). Iterates through the sorted stops and greedily assigns each to the vehicle that can arrive earliest.

### 5. Local-Search Optimization Engine (`src/optimizer.cpp`)
Iteratively improves schedule quality up to `maxIterations`:
- **Phase 1 (Intra-route 2-Opt):** Reverses sub-sequences of stops within each shuttle route to untangle criss-crossing paths.
- **Phase 2 (Inter-shuttle Relocation):** Migrates stops between different vehicles.
- **Feasibility Verification:** Any candidate move is evaluated with `recomputeTimes()`; if any stop arrives past its deadline, the candidate is discarded.
- **Cost Function:** Minimizes combined travel time and wait slack across the fleet.

---

## OOP & DSA Concepts

- **OOP Design:** 4 domain classes (`Config`, `Stop`, `ScheduleEntry`, `ShuttleSchedule`).
- **Operator Overloading:** `Stop::operator<` for `std::sort` compatibility; `operator<<` friend functions for stream printing.
- **Encapsulation:** Internal static helper functions scoping math and string routines.
- **Adjacency Matrix:** 2D dynamic vector (`vector<vector<int>>`) representation of graph travel times.
- **Exception Handling:** `try/catch` wrappers around file streams and numerical parsing.

---

## Project Structure

```
shuttle-optimizer/
├── include/
│   └── shuttle.h           # Domain models, configuration struct, function contracts
├── src/
│   ├── main.cpp            # CLI driver with flag parser & workflow orchestration
│   ├── data.cpp            # CSV ingestion, Haversine formula, Floyd-Warshall matrix
│   ├── scheduler.cpp       # Greedy baseline scheduler (Earliest Deadline First)
│   ├── optimizer.cpp       # Local-search optimizer (2-opt & inter-shuttle relocate)
│   └── evaluator.cpp       # Metrics reporting & CSV export
├── data/
│   └── stops.csv           # Default Dehradun dataset (Clock Tower, ISBT, Railway, Rajpur)
├── web/
│   └── index.html          # Lightweight auto-loading dashboard
├── output/
│   └── final_schedule.csv  # Generated output timetable
├── Makefile                # Build automation
└── README.md               # Technical project documentation
```

---

## Compilation & Build

Compile using GNU Make:
```bash
make
```

Or manually with g++ (C++14):
```bash
g++ -Iinclude -Wall -std=c++14 src/main.cpp src/data.cpp src/scheduler.cpp src/optimizer.cpp src/evaluator.cpp -o shuttle_optimizer.exe
```

---

## Execution & CLI Options

```bash
# Default run (Dehradun dataset, 2 shuttles, 25 km/h, 07:00 start)
.\shuttle_optimizer.exe

# Custom stops file and shuttle count
.\shuttle_optimizer.exe data\stops.csv 2

# Custom speed, start time, and dwell time
.\shuttle_optimizer.exe data\stops.csv 2 --speed 20 --start 06:45 --dwell 3

# Display help
.\shuttle_optimizer.exe --help
```

### CLI Flags Reference

| Option | Argument | Description | Default |
| :--- | :--- | :--- | :--- |
| `--speed` | `<km/h>` | Average shuttle velocity | `25.0` |
| `--start` | `<HH:MM>` | Fleet dispatch start time | `07:00` |
| `--dwell` | `<mins>` | Boarding / dwell time per stop | `2` |
| `--help` | — | Display usage information | — |

---

## Web Dashboard

The project includes an auto-loading dashboard located at `web/index.html`.

1. Double-click `web/index.html` to open it in any browser.
2. The dashboard automatically loads and renders:
   - **Transit Stops Table** (`stops.csv`) with stop IDs, names, coordinates, and deadlines.
   - **Optimized Route Timetable** (`final_schedule.csv`) with vehicle assignments, arrival/departure timings, and on-time status.
   - **Summary KPI Cards** showing stops count, earliest/latest deadlines, active shuttles, and punctuality rate.
3. No manual upload or drag-and-drop required.
