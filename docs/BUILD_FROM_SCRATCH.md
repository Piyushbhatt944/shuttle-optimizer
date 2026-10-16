# Building ShuttleOpt From Scratch

A step-by-step guide to recreating the ShuttleOpt C++ shuttle scheduling program from an empty folder. Each stage explains *why* it matters, *what* you will produce, and shows short, relevant code examples.

---

## Stage 1 — Define the Problem

**Why it matters:** You cannot design a solution until you know exactly what the program must do. Rushing to code without a clear problem statement is the most common student mistake.

A shuttle optimizer solves a real operations problem: given a fleet of shuttles and a set of urban stops, assign shuttles to stops in an order that meets passenger deadlines while minimising wasted travel time.

**Inputs:**
| Input | Description |
|---|---|
| `stops.csv` | Stop ID, name, latitude, longitude, deadline (minutes from now) |
| Fleet size | Number of available shuttles |
| Speed | Average shuttle speed in km/h |

**Outputs:**
- A human-readable schedule (which shuttle visits which stops, at what time)
- A CSV export for record-keeping
- Before/after metrics showing how much the optimizer improved over the greedy baseline

**Why manual scheduling is hard:** Even with 10 stops and 2 shuttles, there are thousands of possible assignments. Deadlines interact — serving stop A first might cause you to miss stop B's deadline. A computer can evaluate millions of orderings per second.

---

## Stage 2 — Design the Solution

**Why it matters:** Algorithm choice determines correctness, speed, and complexity. Document it before you write a single line of code.

### Algorithm choices

| Problem | Algorithm | Reason |
|---|---|---|
| GPS distance | **Haversine formula** | Accounts for Earth's curvature; more accurate than flat Euclidean distance |
| All-pairs travel time | **Floyd-Warshall** | Simple O(n³) DP; fine for ≤ ~50 stops |
| Initial schedule | **Greedy** | Fast baseline — assign the nearest unvisited stop |
| Improvement | **Local search (2-opt + relocate)** | Iteratively swap/move stops to reduce total time |

### Data flow

```
stops.csv
    │
    ▼
loadStops()          ← parse CSV into vector<Stop>
    │
    ▼
buildTravelTimeMatrix()  ← Haversine distances → Floyd-Warshall
    │
    ▼
buildGreedySchedule()    ← nearest-neighbour assignment
    │
    ▼
optimizeSchedule()       ← 2-opt swaps + stop relocation
    │
    ▼
printSchedule()  ──►  console
exportScheduleCSV() ──►  output/schedule.csv
```

---

## Stage 3 — Plan Classes and Functions

**Why it matters:** Designing the data structures before coding prevents costly rewrites later.

### Core types (all in `shuttle.h`)

```cpp
struct Stop {
    int id;
    std::string name;
    double lat, lon;
    int requiredByTime; // deadline in minutes
};

struct ScheduleEntry {
    int stopId;
    int arrivalTime;
    int departureTime;
};

struct ShuttleSchedule {
    int shuttleId;
    std::vector<ScheduleEntry> entries;
};
```

A `Config` namespace holds constants (speed, max iterations, penalty weight) so magic numbers never appear in logic code.

### The five modules

| Module | File | Responsibility |
|---|---|---|
| Data | `data.cpp` | Load CSV, build travel-time matrix |
| Scheduler | `scheduler.cpp` | Greedy initial schedule |
| Optimizer | `optimizer.cpp` | Local search improvement |
| Evaluator | `evaluator.cpp` | Print and export results |
| Main | `main.cpp` | Wire everything together |

### Design decision: `g_stopIdxMap`

Stop IDs in the CSV are arbitrary integers (e.g., 101, 205). If you use them directly as array indices you will either waste enormous memory or crash. `g_stopIdxMap` is a `std::map<int,int>` that translates a stop ID to its position in the `stops` vector. Every matrix lookup uses `g_stopIdxMap[id]` instead of `id` directly.

---

## Stage 4 — Set Up Environment and Repo

**Why it matters:** A clean, version-controlled workspace prevents "it worked on my machine" disasters.

### Install tools
- **MinGW GCC** (≥ 6.3.0) — provides `g++` on Windows
- **VS Code** + the *C/C++* extension (IntelliSense, debugger)

### Create the folder structure

```powershell
mkdir shuttle-optimizer
cd shuttle-optimizer
mkdir include, src, data, output, docs
git init
```

### `.gitignore`

```
*.exe
*.o
output/
```

Committing compiled binaries bloats the repo. The `output/` directory is generated at runtime — never track it.

---

## Stage 5 — Implement Core Features Step by Step

**Why it matters:** Order of implementation matters. Build the shared contract first; never write a caller before you have defined what it calls.

### 5a. `shuttle.h` — shared contract
Define all structs and `extern` declarations here. Every `.cpp` file will `#include "shuttle.h"`.

```cpp
extern std::map<int,int> g_stopIdxMap; // defined once in data.cpp
```

### 5b. `data.cpp` — load and build matrix
Parse each CSV line into a `Stop`, populate `g_stopIdxMap`, then compute the Haversine distance between every pair:

```cpp
double haversine(double lat1, double lon1, double lat2, double lon2) {
    double R = 6371.0;
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dLat/2)*sin(dLat/2) +
               cos(lat1*M_PI/180)*cos(lat2*M_PI/180)*sin(dLon/2)*sin(dLon/2);
    return R * 2 * atan2(sqrt(a), sqrt(1-a));
}
```

After building the distance matrix, run Floyd-Warshall to fill in the shortest travel times between all pairs.

### 5c. `scheduler.cpp` — greedy baseline
Assign stops to shuttles round-robin, then for each shuttle sort its stops by nearest-neighbour:

```cpp
// pick the stop closest to current position
int next = -1;
double best = 1e18;
for (int s : remaining) {
    double d = travelTime[curIdx][g_stopIdxMap[s]];
    if (d < best) { best = d; next = s; }
}
```

### 5d. `evaluator.cpp` — print and export
Format the schedule to `stdout` and write a CSV to `output/schedule.csv`. Create the output directory with `_mkdir` (Windows) if it does not exist.

### 5e. `optimizer.cpp` — local search
Loop until no improvement is found. Try **2-opt** (reverse a sub-sequence within one shuttle's route) and **relocate** (move a single stop to another shuttle). Accept a change only if it lowers the total cost.

### 5f. `main.cpp` — wire together
Call the five modules in data-flow order. Keep `main` thin — it should read arguments, call functions, and handle top-level errors.

---

## Stage 6 — Compile, Debug, and Test

**Why it matters:** Understanding compiler output turns error messages from walls of red text into actionable fixes.

### Build commands

```powershell
# Manual
g++ -Iinclude -Wall -std=c++14 src/main.cpp src/data.cpp src/scheduler.cpp src/optimizer.cpp src/evaluator.cpp -o shuttle_optimizer

# Via Makefile
make        # build
make clean  # remove binary
```

### Common errors

| Error | Likely cause | Fix |
|---|---|---|
| `undefined reference to buildTravelTimeMatrix` | File missing from compile command | Add `src/data.cpp` |
| `'M_PI' was not declared` | GCC strict mode | Add `#define _USE_MATH_DEFINES` before includes |
| Segfault on matrix access | Using raw stop ID as index | Use `g_stopIdxMap[id]` instead |
| `no such file or directory: shuttle.h` | Wrong include path | Pass `-Iinclude` flag |

The `g_stopIdxMap` pattern directly prevents the ID-as-index bug: if your stops have IDs 101–110, accessing `matrix[101][102]` on a 10-element array is undefined behaviour and will crash or silently corrupt data.

---

## Stage 7 — Improve Error Handling and Quality

**Why it matters:** A program that crashes on bad input is not finished — it is just tested with good input.

### Key improvements

```cpp
// File open check
std::ifstream file(path);
if (!file.is_open())
    throw std::runtime_error("Cannot open: " + path);

// Fleet size validation
if (numShuttles <= 0)
    throw std::invalid_argument("numShuttles must be > 0");

// Wrap main logic
try {
    run();
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
}
```

Replace `system("mkdir output")` with `_mkdir("output")` — calling `system()` is a security risk and is platform-dependent. Add checks for empty stop lists before running any algorithm.

---

## Stage 8 — Prepare the Demo

**Why it matters:** A demo proves the program works and shows the optimizer actually adds value over the greedy baseline.

### Default run

```powershell
.\shuttle_optimizer
```

Expected output:
```
=== Greedy Schedule ===
Shuttle 1: Stop A (arr: 0 min) → Stop C (arr: 12 min) → ...
Total cost: 187.4

=== Optimized Schedule ===
Shuttle 1: Stop A (arr: 0 min) → Stop B (arr: 8 min) → ...
Total cost: 143.1   [improvement: 23.7%]
```

### Edge-case run — non-contiguous IDs

Create a test CSV where stop IDs are `201, 305, 410`. Without `g_stopIdxMap` this would crash. With it, the program maps them to indices 0, 1, 2 and runs correctly. Confirm no segfault and that the output CSV appears in `output/`.

### Reading the output

| Section | Meaning |
|---|---|
| Arrival time | Minutes after program start when shuttle reaches stop |
| Deadline met? | Whether arrival ≤ `requiredByTime` |
| Total cost | Weighted sum of travel time + missed deadlines |
| Improvement % | `(greedyCost − optimizedCost) / greedyCost × 100` |

---

*End of guide. For algorithm deep-dives see `docs/ALGORITHMS.md`; for file-by-file reference see `docs/CODE_REFERENCE.md`.*
