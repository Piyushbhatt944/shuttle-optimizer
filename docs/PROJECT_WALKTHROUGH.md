# ShuttleOpt — Project Walkthrough

A beginner-friendly guide to understanding, explaining, and presenting the ShuttleOpt C++ project.

---

## 1. Problem Statement & Objectives

Managing a fleet of campus or city shuttles is harder than it looks. A dispatcher must decide *which shuttle picks up which stop*, in *what order*, so that passengers arrive **before their deadlines** and vehicles don't waste time criss-crossing the city.

ShuttleOpt solves this computationally:

- **Minimise late arrivals** — every stop has a deadline; the system tracks on-time vs. late.
- **Minimise wasted travel time** — routes are refined so shuttles take shorter, smarter paths.
- **Automate the pipeline** — from raw GPS data to a final CSV schedule, with zero manual effort.

---

## 2. Who Uses It

| User | Purpose |
|---|---|
| **Transit managers** | Generate and compare shuttle schedules quickly |
| **CS/DSA students** | Learn how graphs, greedy algorithms, and local search work on a real problem |
| **Teachers / evaluators** | Assess how well students apply OOP and algorithmic thinking |

---

## 3. Features

- 📍 Reads stop locations from a CSV file (`data/stops.csv`)
- 📐 Builds a travel-time matrix using **Haversine distance** + **Floyd-Warshall** shortest paths
- 🚌 Schedules multiple shuttles with a **greedy algorithm**
- 🔁 Refines routes with **local search** (2-opt reversal + inter-shuttle stop relocation)
- 📊 Evaluates and prints an **on-time metrics table**
- 💾 Exports the final schedule to a **CSV file**

---

## 4. Step-by-Step Program Flow

This follows the pipeline in `src/main.cpp`:

```
1. Parse CLI arguments  (config: num_shuttles, speed, etc.)
        ↓
2. loadStops()          (src/data.cpp)   — read stops.csv into g_allStops
        ↓
3. buildTravelTimeMatrix()               — compute pairwise travel times (Haversine + Floyd-Warshall)
        ↓
4. generateBaselineSchedule()  (src/scheduler.cpp)  — greedy assignment
        ↓
5. evaluateSchedule()   (src/evaluator.cpp)  — print before-optimisation metrics
        ↓
6. optimizeSchedule()   (src/optimizer.cpp)  — 2-opt + relocate local search
        ↓
7. evaluateSchedule()                        — print after-optimisation metrics
        ↓
8. exportScheduleToCSV()                     — write results to output file
```

---

## 5. File and Function Roles

**`include/shuttle.h`** — The backbone of the project. Declares all domain models: `Config` (runtime settings), `Stop` (id, name, latitude, longitude, deadline), `ScheduleEntry` (stop + arrival time), and `ShuttleSchedule` (a list of entries for one shuttle). Also declares the global variables `g_allStops` and `g_stopIdxMap` used as shared state across modules.

**`src/data.cpp`** — Handles all data loading and graph construction. `loadStops()` opens `data/stops.csv`, parses each line with `stoi`/`stod`, and populates `g_allStops` and `g_stopIdxMap`. `buildTravelTimeMatrix()` then computes Haversine distances between every pair of stops, converts them to travel times, and runs Floyd-Warshall to find shortest all-pairs paths.

**`src/scheduler.cpp`** — Implements `generateBaselineSchedule()`, the greedy first-pass algorithm. It sorts stops by deadline (earliest first), then for each stop finds the shuttle that can arrive there the soonest, and assigns it. The result is a valid (though not yet optimal) schedule.

**`src/optimizer.cpp`** — Implements `optimizeSchedule()`, the local search refinement. Phase 1 applies **2-opt**: for each shuttle's route, it tries reversing every sub-segment to see if travel time decreases. Phase 2 applies **relocate**: it tries moving each stop from its current shuttle to a different one for a better overall time. Uses `g_stopIdxMap` for safe matrix lookups.

**`src/evaluator.cpp`** — Contains `evaluateSchedule()`, which prints a formatted table showing each stop, its scheduled arrival, deadline, and on-time/late status. Also contains `exportScheduleToCSV()`, which writes the same data to a CSV file using `ofstream`.

---

## 6. Key C++ Concepts Used

| Concept | Where used |
|---|---|
| Classes with constructors | `Stop`, `Config`, `ShuttleSchedule` in `shuttle.h` |
| `operator<` overloading | Sorting stops by deadline |
| `operator<<` (friend) | Printing `Stop`/`ScheduleEntry` to `cout` |
| `extern` globals | `g_allStops`, `g_stopIdxMap` declared in `shuttle.h`, defined in `data.cpp` |
| `vector`, `map`, `sort`, `reverse` | Throughout scheduling and optimisation |
| Range-based `for` loop | Iterating stops and schedule entries |
| `try / catch` | File I/O error handling in `data.cpp` |
| `stoi` / `stod` | Parsing CSV strings in `loadStops()` |
| `ifstream` / `ofstream` | Reading stops.csv, writing output CSV |
| `printf` / `snprintf` | Formatted table output in `evaluator.cpp` |
| `#ifdef` | Platform-specific code guards |

---

## 7. Key Algorithms

### Haversine Formula
Straight-line (Euclidean) distance fails on a sphere. Haversine computes the **great-circle distance** between two GPS coordinates:

```
a = sin²(Δlat/2) + cos(lat1)·cos(lat2)·sin²(Δlon/2)
distance = 2 · R · arcsin(√a)       // R = 6371 km
```

Travel time = distance ÷ speed (25 km/h). Implemented in `src/data.cpp`.

### Floyd-Warshall (All-Pairs Shortest Path)
After building direct travel times, Floyd-Warshall finds the **shortest path between every pair of stops**, even if going through intermediate stops is faster:

```cpp
for (int k = 0; k < n; k++)          // intermediate stop
  for (int i = 0; i < n; i++)        // source
    for (int j = 0; j < n; j++)      // destination
      dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j]);
```

Time complexity: O(n³). Works fine for small n (4 stops here).

### Greedy Scheduling
Sort stops by deadline. For each stop, assign it to whichever shuttle can reach it earliest:

```cpp
// pseudocode from src/scheduler.cpp
sort(stops, by deadline);
for each stop:
    best_shuttle = shuttle with minimum arrival_time_at(stop)
    assign stop to best_shuttle
```

Fast but not globally optimal — that's why we optimise next.

### 2-opt (Route Reversal)
For a shuttle visiting stops `[A, B, C, D]`, try reversing every sub-segment (e.g. `[A, C, B, D]`) and keep the version with the shorter total travel time. Repeats up to `MAX_ITERATIONS = 500` times.

### Relocate (Inter-Shuttle Move)
Try removing a stop from one shuttle's route and inserting it into another shuttle's route at the best position. If total travel time improves, keep the move.

---

## 8. Key Data Structures

| Structure | Type | Purpose |
|---|---|---|
| Travel-time matrix | `vector<vector<double>>` | Adjacency matrix of travel times between all stop pairs |
| Stop list | `vector<Stop>` (`g_allStops`) | All stops loaded from CSV |
| ID → index map | `map<int,int>` (`g_stopIdxMap`) | Converts stop IDs to matrix row/column indices |
| Schedule | `vector<ShuttleSchedule>` | Each shuttle holds a `vector<ScheduleEntry>` |

---

## 9. Important Code Snippets

**Haversine distance core (src/data.cpp):**
```cpp
double dLat = toRad(lat2 - lat1);
double dLon = toRad(lon2 - lon1);
double a = sin(dLat/2)*sin(dLat/2)
         + cos(toRad(lat1))*cos(toRad(lat2))*sin(dLon/2)*sin(dLon/2);
double c = 2 * atan2(sqrt(a), sqrt(1 - a));
return 6371.0 * c;  // km
```

**Floyd-Warshall triple loop (src/data.cpp):**
```cpp
for (int k = 0; k < n; k++)
  for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
      if (dist[i][k] + dist[k][j] < dist[i][j])
        dist[i][j] = dist[i][k] + dist[k][j];
```

**Greedy assignment (src/scheduler.cpp):**
```cpp
int bestShuttle = 0;
double bestArrival = INF;
for (int s = 0; s < numShuttles; s++) {
    double arrival = shuttleReadyTime[s] + travelTime(lastStop[s], stop);
    if (arrival < bestArrival) { bestArrival = arrival; bestShuttle = s; }
}
schedules[bestShuttle].entries.push_back({stop, bestArrival});
```

**Safe matrix lookup via g_stopIdxMap (src/optimizer.cpp):**
```cpp
int idxA = g_stopIdxMap[stopA.id];
int idxB = g_stopIdxMap[stopB.id];
double t = travelMatrix[idxA][idxB];
```

---

## 10. Why These Implementation Choices

- **Haversine, not Euclidean** — GPS coordinates are on a sphere; flat-plane distance introduces significant error even over short distances.
- **Floyd-Warshall, not Dijkstra** — With only 4 stops, O(n³) is negligible. Floyd-Warshall gives *all* pairs in one pass, which the scheduler needs.
- **Greedy first, then local search** — A pure local search starting from scratch can get stuck. Greedy provides a solid initial solution; local search then polishes it efficiently without exploring the full exponential search space.

---

## 11. Current Limitations

- 🚗 **Fixed speed** — All shuttles travel at 25 km/h regardless of traffic or road type.
- 👥 **No passenger capacity** — Shuttles can be assigned unlimited stops.
- 📡 **No real-time updates** — The schedule is static; delays or new stops require a full re-run.
- 📦 **Small dataset** — Only 4 stops in `data/stops.csv`; scalability to hundreds of stops is untested.

---

## 12. Key Concepts to Study Before Viva

1. **Haversine formula** — why and how it computes spherical distance
2. **Floyd-Warshall algorithm** — triple-loop logic, time complexity O(n³), what "relaxation" means
3. **Greedy algorithms** — definition, when they work well, when they don't
4. **Local search / 2-opt** — concept of neighbourhood, how reversals improve a route
5. **Adjacency matrix** — how a 2D vector represents a weighted graph
6. **`extern` and global variables** — how `g_allStops` and `g_stopIdxMap` are shared across `.cpp` files
7. **STL containers** — `vector` vs `map`, when to use each
8. **Operator overloading** — purpose of overloading `<` and `<<` in this project
9. **File I/O in C++** — `ifstream` for reading, `ofstream` for writing, error handling with `try/catch`
10. **Separation of concerns** — why the project splits data, scheduling, optimisation, and evaluation into different files
