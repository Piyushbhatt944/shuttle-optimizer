# ShuttleOpt — Algorithmic Urban Transit Scheduling & Route Optimization Engine

**ShuttleOpt** is an end-to-end transit dispatch and schedule optimization system developed in **C++14**, paired with a lightweight **Python HTTP server** and a responsive **Web Dashboard**. The system accepts transit stop locations (GPS coordinates) with strict passenger arrival deadlines, models urban geography using spherical trigonometry and dynamic programming graph algorithms, builds an initial schedule using a greedy heuristic, and refines it via local search metaheuristics (intra-route 2-Opt and inter-route relocation).

This document serves as the complete technical manual for developers, students, researchers, and LLM coding assistants working with or evaluating this codebase.

---

## Table of Contents
1. [System Overview & Architecture](#1-system-overview--architecture)
2. [Complete File-by-File Technical Breakdown](#2-complete-file-by-file-technical-breakdown)
3. [Core Algorithms & Mathematical Formulations](#3-core-algorithms--mathematical-formulations)
4. [Data Formats & Storage Schemas](#4-data-formats--storage-schemas)
5. [End-to-End Execution & Data Flow](#5-end-to-end-execution--data-flow)
6. [Interactive Web Workflow & API Specification](#6-interactive-web-workflow--api-specification)
7. [Compilation, Execution & CLI Reference](#7-compilation-execution--cli-reference)
8. [Comprehensive Study & Viva Preparation Guide](#8-comprehensive-study--viva-preparation-guide)

---

## 1. System Overview & Architecture

### High-Level Architectural Flowchart

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           STORAGE & DATA SOURCES                            │
│  data/stops.csv ───► data/dehradun_samples.csv ───► output/final_schedule.csv│
└───────────────────────┬───────────────────────────────────▲─────────────────┘
                        │                                   │
┌───────────────────────▼───────────────────────────────────┴─────────────────┐
│                          CORE C++ SCHEDULING ENGINE                         │
│                                                                             │
│  1. Ingestion (data.cpp)                                                    │
│     Parse stops.csv ──► Build ID-to-Index Map (g_stopIdxMap)                │
│                                                                             │
│  2. Spatial Graph (data.cpp)                                                │
│     Haversine Matrix (km) ──► Speed Scaling (min) ──► Floyd-Warshall APSP   │
│                                                                             │
│  3. Greedy Baseline (scheduler.cpp)                                         │
│     Sort by deadline (operator<) ──► Assign to Earliest-Arrival Shuttle      │
│                                                                             │
│  4. Local Search Optimizer (optimizer.cpp)                                  │
│     Intra-route 2-Opt ──► Inter-route Relocation ──► Feasibility & Cost Check│
│                                                                             │
│  5. Evaluation & Export (evaluator.cpp)                                     │
│     Compute Travel, Wait, Punctuality ──► Stream to output/final_schedule.csv│
└───────────────────────▲─────────────────────────────────────────────────────┘
                        │ Subprocess Call (subprocess.run)
┌───────────────────────┴─────────────────────────────────────────────────────┐
│                          BACKEND & WEB INTERFACE                            │
│                                                                             │
│  server.py (Python 3 HTTP Server @ localhost:8000)                          │
│     ├── Static file routing (/ -> web/index.html, /data/, /output/)         │
│     ├── CORS Headers (Access-Control-Allow-Origin: *)                       │
│     └── POST /api/add-stop:                                                 │
│           Validate duplicate stop ──► Append to stops.csv ──► Run C++ Engine│
│                                                                             │
│  web/index.html (Client-Side Dashboard)                                     │
│     ├── Auto-fetch data/stops.csv & output/final_schedule.csv (no drag/drop)│
│     ├── Dropdown selection from dehradun_samples.csv (S1-S4)                │
│     ├── Duplicate stop detection & prevention                               │
│     └── Dynamic KPI summary cards & timetable rendering                     │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Complete File-by-File Technical Breakdown

### `include/shuttle.h` (Domain Models & Header Contract)
The central header declaring classes, configuration structs, global states, and functional contracts.
- **`struct Config`**: Encapsulates runtime parameters:
  - `int startTime`: Fleet departure time in minutes since midnight (default: `420` = 07:00).
  - `int dwellTime`: Boarding and alighting duration per stop in minutes (default: `2`).
  - `int maxIterations`: Iteration cap for local search optimization (default: `500`).
  - `double speed`: Average shuttle velocity in km/h (default: `25.0`).
- **`class Stop`**: Represents an individual transit station:
  - Fields: `int id`, `string name`, `double lat`, `double lon`, `int requiredByTime`.
  - Constructors: Default `Stop()` and parameterized `Stop(id, name, lat, lon, req)`.
  - Operator Overloading: `bool operator<(const Stop& other) const` compares `requiredByTime < other.requiredByTime` to enable automated sorting by deadline using `std::sort`.
  - Friend Function: `operator<<(ostream& os, const Stop& s)` for debugging and stream formatting.
- **`class ScheduleEntry`**: Represents an assigned visit to a stop by a specific shuttle:
  - Fields: `int stopId`, `int arrivalTime`, `int departureTime`.
  - Friend Function: `operator<<` for formatted console logging.
- **`class ShuttleSchedule`**: Represents the complete itinerary for one shuttle vehicle:
  - Fields: `int shuttleId`, `vector<ScheduleEntry> stops`.
  - Destructor: `~ShuttleSchedule()` cleanly empties the internal stops vector.
- **Global Variables (extern)**:
  - `extern Config g_config;`: Global runtime configuration instance.
  - `extern vector<Stop> g_allStops;`: Cached vector of all active stops.
  - `extern map<int, int> g_stopIdxMap;`: Lookup table mapping arbitrary stop IDs to contiguous matrix indices (`0` to `N-1`).

### `src/main.cpp` (CLI Driver & Pipeline Orchestration)
The command-line interface entry point.
- **CLI Flag Parser**: Parses command-line tokens supporting both positional arguments and flags:
  - `--speed <km/h>`: Updates `g_config.speed`.
  - `--start <HH:MM>`: Converts a 24-hour string into integer minutes since midnight (`parseHHMM()`) and updates `g_config.startTime`.
  - `--dwell <mins>`: Updates `g_config.dwellTime`.
  - `--help` / `-h`: Displays command options.
- **Workflow Orchestration**:
  1. Validates `numShuttles >= 1`.
  2. Ingests stop data via `loadStops(stopsFile)`.
  3. Computes the spatial graph via `buildTravelTimeMatrix(stops)`.
  4. Generates initial timetable via `generateBaselineSchedule()`.
  5. Evaluates and prints the baseline metrics via `evaluateSchedule()`.
  6. Optimizes routes via `optimizeSchedule()`.
  7. Evaluates and logs optimized metrics.
  8. Ensures output folder existence (`ensureOutputDir()` using native `_mkdir` on Windows and `mkdir` on POSIX).
  9. Streams final itinerary to `output/final_schedule.csv`.

### `src/data.cpp` (CSV Parsing & Graph Construction)
Implements file input handling, coordinate trigonometry, and shortest path matrix construction.
- **`loadStops(filename)`**:
  - Opens file using `std::ifstream`.
  - Skips CSV header and tokenizes each comma-separated line using `std::istringstream`.
  - Uses `trimRight()` to remove trailing spaces, carriage returns (`\r`), and newlines (`\n`).
  - Populates `g_allStops` and establishes `g_stopIdxMap`:
    ```cpp
    for (int i = 0; i < stops.size(); i++) {
        g_stopIdxMap[stops[i].id] = i;
    }
    ```
- **`toRadians(degrees)`**: Converts decimal degrees to radians ($\text{rad} = \text{deg} \times \frac{\pi}{180}$).
- **`haversineDistanceKm(lat1, lon1, lat2, lon2)`**: Calculates great-circle distance between two geographic coordinates using $R = 6371.0\text{ km}$.
- **`buildTravelTimeMatrix(stops)`**:
  - Initializes an $N \times N$ matrix where `matrix[i][i] = 0`.
  - For $i \ne j$, computes Haversine distance, divides by `g_config.speed`, converts to minutes, and applies `max(1, static_cast<int>(round(timeMin)))`.
  - Runs the **Floyd-Warshall algorithm** ($O(V^3)$ triple loop) to ensure shortest paths and triangle inequality compliance.

### `src/scheduler.cpp` (Greedy Baseline Timetable)
Implements the initial constructive heuristic.
- **`generateBaselineSchedule(stops, travelTimes, numShuttles)`**:
  - Copies and sorts stops chronologically using `std::sort(sortedStops.begin(), sortedStops.end())` (Earliest Deadline First).
  - Maintains `currentTime` vector initialized to `g_config.startTime` and `lastStopId` initialized to `-1` for each shuttle.
  - Iterates over sorted stops: for each shuttle, calculates projected arrival:
    - If shuttle has not moved (`lastStopId == -1`): $\text{arrival} = \text{startTime}$.
    - If shuttle has previous stops: $\text{arrival} = \text{currentTime} + \text{travelTimes}[\text{lastIdx}][\text{stopIdx}]$.
  - Greedily commits the stop to the shuttle achieving the minimum arrival time.
  - Updates shuttle's clock: $\text{currentTime} = \text{arrival} + \text{dwellTime}$.

### `src/optimizer.cpp` (Local Search Optimization Metaheuristic)
Refines the baseline schedule to minimize travel time and unnecessary wait slack.
- **`getRequiredByTime(stopId)`**: Retrieves deadline for any given stop ID from `g_allStops`.
- **`recomputeTimes(schedule, travelTimes)`**:
  - Recalculates arrival and departure timings for a modified shuttle route.
  - **Feasibility Gatekeeper**: Returns `false` immediately if $\text{arrival} > \text{requiredByTime}$ for any stop along the route.
- **`computeTotalCost(schedules, travelTimes)`**:
  - Objective function: $\text{Cost} = \text{Total Travel Time} + \text{Total Wait Slack}$, where $\text{slack} = \text{deadline} - \text{arrival}$.
- **`optimizeSchedule(baseline, travelTimes)`**:
  - Iterates while improvements occur and `iterations < g_config.maxIterations`.
  - **Phase 1 (Intra-Route 2-Opt)**: For each shuttle with $\ge 2$ stops, tests all index pairs $(i, j)$ and reverses the sub-sequence using `std::reverse`. If feasible and cost decreases, accepts the candidate.
  - **Phase 2 (Inter-Route Relocation)**: If 2-Opt yields no improvement, tests migrating a stop from shuttle $s_1$ at position $i$ to shuttle $s_2$ at position $j$. Evaluates feasibility on both vehicles and adopts the move if combined cost drops.

### `src/evaluator.cpp` (Punctuality Reporting & Export)
Provides metrics calculation and CSV serialization.
- **`minutesToHHMM(minutes)`**: Formats integer minutes since midnight into 24-hour `HH:MM` format.
- **`getStopName(stopId)`**: Resolves a stop ID to its human-readable name.
- **`evaluateSchedule(schedule, label)`**:
  - Prints a console table displaying: Stop Name, Arrival, Departure, Deadline, and Status (`ON TIME` vs `** LATE **`).
  - Aggregates and logs: Total Travel Time, Total Wait/Slack Time, Combined Cost, and On-Time Percentage.
- **`exportScheduleToCSV(schedule, filename)`**:
  - Opens `std::ofstream` and writes header: `shuttle_id,stop_id,stop_name,arrival_time,departure_time`.
  - Writes sequential rows formatted with zero-padded `HH:MM` times.

### `server.py` (Local Backend API Server)
A lightweight HTTP server built using Python's standard library (`http.server`, `socketserver`, `subprocess`, `csv`, `json`).
- Runs on `http://localhost:8000`.
- **CORS Handling**: Overrides `end_headers()` to attach `Access-Control-Allow-Origin: *`, `Access-Control-Allow-Methods`, and handles HTTP `OPTIONS` requests, allowing requests from both `http://localhost:8000` and `file:///` browser contexts.
- **Static File Routing**: Routes `/` to `/web/index.html` and serves files from the repository root.
- **API Endpoint (`POST /api/add-stop`)**:
  - Parses JSON: `{"name": str, "lat": float, "lon": float, "deadline": int}`.
  - **Duplicate Validation**: Reads `data/stops.csv` and raises an HTTP 400 error if a stop with the same name already exists.
  - **ID Generation**: Reads existing IDs and assigns $\text{next\_id} = \max(\text{existing IDs}) + 1$.
  - **Data Persistence**: Appends the new record to `data/stops.csv`.
  - **Trigger C++ Optimizer**: Calls `subprocess.run(["./shuttle_optimizer.exe", "data/stops.csv", "2"])` to recompute `output/final_schedule.csv`.
  - Returns `{"success": true, "output": ...}`.

### `web/index.html` (Interactive Web Dashboard)
Single-page frontend application designed with zero external build dependencies.
- **Zero-Interaction Auto-Loading**:
  - On `DOMContentLoaded`, calls `loadData()`.
  - Uses cache-busting queries (`?t=Date.now()`) to query `/data/stops.csv` and `/output/final_schedule.csv`.
  - Multi-tier fallback: Tries fetching from relative server path, local relative path, and finally embedded HTML `<script type="text/csv">` tags so it never renders blank.
- **Sample Stop Insertion UI**:
  - Fetches `data/dehradun_samples.csv` and populates a selection dropdown (`<select>`).
  - Associates latitude and longitude automatically in the background.
  - Disables options for stops that are already present in the active schedule.
  - Submits to `http://localhost:8000/api/add-stop` and triggers automatic page reload on success.
- **Visual Timetable & Metrics Cards**:
  - Displays summary KPI cards (Total Stops, Region, Deadlines, Active Shuttles, Punctuality).
  - Renders tables for both raw stops and vehicle schedules with status badges.

### `Makefile` (Build System)
Build automation script for GNU Make:
- `CXX = g++`
- `CXXFLAGS = -Iinclude -Wall -std=c++14`
- Default target `all` links `src/main.cpp`, `src/data.cpp`, `src/scheduler.cpp`, `src/optimizer.cpp`, and `src/evaluator.cpp` into `shuttle_optimizer.exe`.
- `clean` target safely deletes the executable cross-platform (`del` on Windows / `rm` on POSIX).

---

## 3. Core Algorithms & Mathematical Formulations

### 1. Haversine Spatial Distance Formula
Computes spherical geodesic distance between two points on Earth given their latitudes and longitudes $(\phi_1, \lambda_1)$ and $(\phi_2, \lambda_2)$:

$$\Delta \phi = \frac{\pi}{180} (\text{lat}_2 - \text{lat}_1), \quad \Delta \lambda = \frac{\pi}{180} (\text{lon}_2 - \text{lon}_1)$$

$$a = \sin^2\left(\frac{\Delta \phi}{2}\right) + \cos\left(\frac{\pi}{180} \text{lat}_1\right) \cos\left(\frac{\pi}{180} \text{lat}_2\right) \sin^2\left(\frac{\Delta \lambda}{2}\right)$$

$$c = 2 \cdot \text{atan2}\left(\sqrt{a}, \sqrt{1 - a}\right)$$

$$d = R \cdot c \quad (\text{where } R = 6371.0 \text{ km})$$

$$\text{Travel Time (minutes)} = \max\left(1, \text{round}\left(\frac{d}{\text{Speed}} \times 60\right)\right)$$

### 2. Floyd-Warshall All-Pairs Shortest Path (APSP)
Operates on the $N \times N$ adjacency matrix $D$:

$$D^{(k)}[i][j] = \min\left(D^{(k-1)}[i][j], \; D^{(k-1)}[i][k] + D^{(k-1)}[k][j]\right)$$

- **Time Complexity**: $O(V^3)$
- **Space Complexity**: $O(V^2)$
- **Role in Project**: Guarantees triangular inequality across transit hubs, preventing geometric shortcuts from distorting schedule costs.

### 3. Earliest Deadline First (EDF) Greedy Dispatcher
- Sorts stops such that $S_1 \le S_2 \le \dots \le S_N$ based on $\text{requiredByTime}$.
- For each stop $S_i$ and each vehicle $v \in \{0, \dots, K-1\}$:
  $$\text{Arr}(v, S_i) = \begin{cases} \text{startTime}, & \text{if route}(v) = \emptyset \\ \text{currTime}(v) + \text{matrix}[\text{last}(v)][S_i], & \text{otherwise} \end{cases}$$
  $$v^* = \arg\min_v \text{Arr}(v, S_i)$$
- **Time Complexity**: $O(N \log N + N \cdot K)$

### 4. 2-Opt Local Search (Intra-Route Edge Exchange)
For a shuttle route $(v_0, v_1, \dots, v_i, \dots, v_j, \dots, v_m)$, 2-Opt reverses the sub-tour between $i$ and $j$:
$$\text{candidate route} = (v_0, \dots, v_{i-1}, v_j, v_{j-1}, \dots, v_i, v_{j+1}, \dots, v_m)$$
If $\text{recomputeTimes}(\text{candidate}) == \text{true}$ and $\text{Cost}(\text{candidate}) < \text{Cost}(\text{current})$, the move is committed.

### 5. Inter-Shuttle Stop Relocation
Moves stop $v$ from shuttle $s_1$ at index $i$ to shuttle $s_2$ at index $j$:
$$\text{route}(s_1)' = \text{route}(s_1) \setminus \{v\}, \quad \text{route}(s_2)' = \text{insert}(\text{route}(s_2), j, v)$$
Accepted if both routes maintain feasibility and cumulative system cost decreases.

---

## 4. Data Formats & Storage Schemas

### Active Stops File (`data/stops.csv`)
Stores all stops currently assigned to the transit network.
```csv
id,name,lat,lon,requiredByTime
0,Clock Tower,30.3255,78.0421,450
1,ISBT Dehradun,30.3472,78.0326,465
2,Dehradun Railway Station,30.3182,78.0325,435
3,Rajpur Road,30.3350,78.0508,480
```
- `id`: Non-negative integer identifier.
- `name`: Text label of the station.
- `lat`, `lon`: GPS coordinates in decimal degrees.
- `requiredByTime`: Integer minutes since 00:00 (e.g., $450 = 07:30\text{ AM}$).

### Sample Stops File (`data/dehradun_samples.csv`)
Stores pre-configured Dehradun locations available for insertion via the UI.
```csv
ID,Name,Latitude,Longitude
S1,Pacific Mall,30.3674,78.0673
S2,FRI Dehradun,30.3444,77.9995
S3,Rispana Bridge,30.2980,78.0560
S4,Ballupur Chowk,30.3398,78.0165
```

### Final Schedule Output (`output/final_schedule.csv`)
Generated by `shuttle_optimizer.exe`.
```csv
shuttle_id,stop_id,stop_name,arrival_time,departure_time
1,1,ISBT Dehradun,07:00,07:02
1,2,Dehradun Railway Station,07:10,07:12
1,3,Rajpur Road,07:18,07:20
1,0,Clock Tower,07:23,07:25
```

---

## 5. End-to-End Execution & Data Flow

### Step-by-Step Execution Lifecycle

```
[1. User enters command or interacts with Web UI]
                        │
                        ▼
[2. server.py receives POST request / User runs CLI]
                        │
                        ▼
[3. C++ binary executed: ./shuttle_optimizer.exe data/stops.csv 2]
                        │
                        ▼
[4. src/data.cpp: loadStops()]
    - Reads data/stops.csv
    - Builds vector<Stop> g_allStops
    - Builds g_stopIdxMap { stopID -> matrixIndex }
                        │
                        ▼
[5. src/data.cpp: buildTravelTimeMatrix()]
    - Calculates Haversine distances
    - Converts km to minutes using g_config.speed
    - Runs Floyd-Warshall APSP to guarantee metric triangle inequality
                        │
                        ▼
[6. src/scheduler.cpp: generateBaselineSchedule()]
    - Sorts stops by requiredByTime (EDF)
    - Greedily assigns each stop to earliest-reaching shuttle
                        │
                        ▼
[7. src/optimizer.cpp: optimizeSchedule()]
    - Phase 1: 2-Opt intra-shuttle route inversion
    - Phase 2: Inter-shuttle stop relocation
    - Evaluates feasibility (arrival <= deadline)
    - Minimizes total travel + wait slack
                        │
                        ▼
[8. src/evaluator.cpp: evaluateSchedule() & exportScheduleToCSV()]
    - Prints formatted timetable & punctuality to stdout
    - Writes output/final_schedule.csv
                        │
                        ▼
[9. Web Dashboard: web/index.html auto-reloads and renders updated schedule]
```

---

## 6. Interactive Web Workflow & API Specification

### Endpoint: `POST /api/add-stop`
- **Request Headers**: `Content-Type: application/json`
- **Request Body**:
  ```json
  {
    "name": "Pacific Mall",
    "lat": 30.3674,
    "lon": 78.0673,
    "deadline": 500
  }
  ```
- **Response (Success - HTTP 200)**:
  ```json
  {
    "success": true,
    "output": "============================================================\n  ShuttleOpt\n..."
  }
  ```
- **Response (Duplicate Error - HTTP 400)**:
  ```json
  {
    "success": false,
    "error": "Stop 'Pacific Mall' has already been added to the schedule."
  }
  ```

---

## 7. Compilation, Execution & CLI Reference

### Build Instructions
```bash
# Compile using Makefile (MinGW / GCC)
make

# Manual compilation
g++ -Iinclude -Wall -std=c++14 src/main.cpp src/data.cpp src/scheduler.cpp src/optimizer.cpp src/evaluator.cpp -o shuttle_optimizer.exe
```

### CLI Command Options
```bash
# Standard execution (Default dataset, 2 shuttles)
.\shuttle_optimizer.exe data\stops.csv 2

# Custom speed, start time, and dwell time
.\shuttle_optimizer.exe data\stops.csv 2 --speed 30 --start 06:30 --dwell 3

# View help
.\shuttle_optimizer.exe --help
```

### Running the Full Web Server
```powershell
python server.py
# Server starts at http://localhost:8000
```
Open **`http://localhost:8000`** in any modern web browser.

---

## 8. Comprehensive Study & Viva Preparation Guide

### A. Key C++ & Object-Oriented Programming (OOP) Concepts
1. **Classes vs. Structs**:
   - `class Stop`, `class ScheduleEntry`, `class ShuttleSchedule` encapsulate data with constructor initialization lists and overloaded operators.
   - `struct Config` models public runtime configuration without boilerplate getters/setters.
2. **Operator Overloading**:
   - `bool Stop::operator<(const Stop& other) const`: Strict weak ordering overload enabling standard library algorithms (`std::sort`) to sort stops by deadline directly.
   - `friend ostream& operator<<(ostream& os, const Class& obj)`: Stream insertion operator overloading allowing object serialization to console or file streams without violating encapsulation.
3. **Friend Functions**:
   - Declared inside `Stop`, `ScheduleEntry`, and `ShuttleSchedule` to grant `operator<<` access to private/protected members while keeping internal state intact.
4. **Static Members & Functions**:
   - `static double toRadians()`, `static double haversineDistanceKm()`, and `static void trimRight()` use internal linkage (`static` at file scope) in `data.cpp`, preventing namespace collisions across translation units.
5. **Global Storage via `extern`**:
   - `extern vector<Stop> g_allStops;` and `extern map<int, int> g_stopIdxMap;` are declared in `include/shuttle.h` and allocated exactly once in `src/data.cpp`, establishing shared application state across modules.
6. **Exception Handling**:
   - File I/O operations (`loadStops`) throw `std::runtime_error` when files are missing or unreadable; `main.cpp` catches these exceptions gracefully, preventing abnormal process termination.

### B. Data Structures & Algorithms (DSA) Concepts
1. **Adjacency Matrix Graph Representation**:
   - Represented as `vector<vector<int>>`. Cell `[i][j]` stores the directed travel duration in minutes between stop index $i$ and stop index $j$.
2. **Hash / Balanced Tree Lookup (`std::map<int, int>`)**:
   - Maps arbitrary stop IDs to contiguous matrix row/column indices ($0$ to $N-1$). Prevents array index out-of-bounds errors when IDs are non-contiguous.
3. **Dynamic Programming (Floyd-Warshall)**:
   - Evaluates all intermediate vertices $k$ to compute all-pairs shortest paths in $O(V^3)$ time, establishing mathematical triangle inequality across the transit network.
4. **Greedy Heuristic (Earliest Deadline First)**:
   - A constructive dispatch algorithm making irrevocable local choices at each step, assigning each deadline-sorted stop to whichever shuttle can arrive earliest.
5. **Local Search Metaheuristics (2-Opt & Relocation)**:
   - Hill-climbing optimization exploring the immediate neighborhood of a valid schedule. Continues iterating until no cost-reducing feasible swap can be found or `maxIterations` is exhausted.

### C. Likely Viva Questions & Technical Model Answers

#### Q1: Why did you use the Haversine formula instead of Euclidean distance ($\sqrt{\Delta x^2 + \Delta y^2}$)?
> **Answer**: Euclidean distance assumes a flat 2D Cartesian plane. Because the Earth is an oblate spheroid, calculating distances from latitude and longitude without accounting for planetary curvature introduces significant geometric distortion. The Haversine formula calculates the true great-circle distance over a sphere of radius $R = 6371.0\text{ km}$, providing accurate inter-stop kilometers for urban transit calculations.

#### Q2: What is the purpose of `g_stopIdxMap`? What bug would occur without it?
> **Answer**: In real-world datasets, stop IDs are often non-contiguous (e.g., stops 10, 25, 42). If the adjacency matrix is sized to the number of stops ($N \times N$), attempting to index `matrix[stop.id]` directly would access memory outside the vector bounds, causing segmentation faults or memory corruption. `g_stopIdxMap` (`std::map<int, int>`) maps any arbitrary stop ID to a continuous index between $0$ and $N-1$, ensuring safe matrix access.

#### Q3: Why is Floyd-Warshall loop order $k \to i \to j$ critical? Can the $k$ loop be placed inside?
> **Answer**: No, $k$ must be the outermost loop. Floyd-Warshall is a dynamic programming algorithm where state $D^{(k)}[i][j]$ represents the shortest path from $i$ to $j$ using only intermediate nodes from the set $\{0, 1, \dots, k\}$. Placing $k$ as the outer loop ensures that all-pairs paths are fully optimized using vertex $k$ before vertex $k+1$ is introduced. Placing $k$ inside would compute incomplete paths.

#### Q4: Why does the optimizer sometimes put all 4 stops on one shuttle instead of using both shuttles?
> **Answer**: The objective function minimizes $\text{Cost} = \text{Travel Time} + \text{Wait Slack}$, where wait slack is $\text{deadline} - \text{arrival}$. When a single shuttle visits all stops sequentially in a well-ordered path, arrivals occur closer to their deadlines, which mathematically decreases cumulative wait slack. Because all deadlines are still met ($100\%$ on-time) and no penalty was assigned to idle vehicles, the unconstrained optimizer selects the single-shuttle solution as having the lowest mathematical cost. To enforce equal vehicle utilization, a vehicle activation penalty or capacity constraint can be added.

#### Q5: What is the time complexity of the complete pipeline?
> **Answer**:
> - Matrix Construction: $O(N^2)$ Haversine calculations $+ O(N^3)$ Floyd-Warshall.
> - Baseline Scheduling: $O(N \log N)$ for deadline sorting $+ O(N \cdot K)$ for greedy assignment across $K$ shuttles.
> - Optimization Iteration: $O(N^2)$ for 2-Opt $+ O(N^2 \cdot K^2)$ for inter-shuttle relocation.
> - Total Execution Time for $N=4$ stops and $K=2$ shuttles is under 5 milliseconds.

#### Q6: How does the client-server integration handle cross-origin issues when opened via `file:///`?
> **Answer**: Modern browsers enforce strict CORS and file-origin restrictions that block standard relative `fetch()` calls on `file:///` URLs. In `server.py`, we explicitly configure HTTP response headers with `Access-Control-Allow-Origin: *` and handle preflight `OPTIONS` requests. In `web/index.html`, JavaScript detects if `window.location.protocol === 'file:'` and routes API requests directly to `http://localhost:8000/api/add-stop`, allowing seamless local execution.
