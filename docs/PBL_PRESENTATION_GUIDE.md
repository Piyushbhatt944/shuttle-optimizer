# ShuttleOpt — PBL Presentation Guide

> A reference for the project group to present, demo, and defend **ShuttleOpt** to teachers.

---

## SECTION A — Presentation Script (Slide Talking Points)

### Slide 1 — Problem Statement
- Urban shuttle scheduling is done manually by operators — error-prone and inefficient.
- Each shuttle stop has a **deadline**: the latest time a shuttle must arrive. Missing it means passengers are stranded.
- With multiple shuttles and stops, manually finding the best assignment is infeasible even for small fleets.
- **Core question:** Can an automated system assign shuttles to stops, respect deadlines, and minimise total travel and wait time?

### Slide 2 — Motivation & Objectives
**Objectives:**
1. Parse real-world stop data (GPS coordinates, deadlines) from a CSV file.
2. Compute accurate travel times between all stop pairs using the **Haversine formula**.
3. Produce an initial schedule greedily sorted by deadline.
4. Refine the schedule with local search to reduce total cost while keeping all stops on time.

### Slide 3 — Proposed Solution (Pipeline)
```
CSV Input → Haversine Matrix → Floyd-Warshall → Greedy Schedule → Local Search → CSV Export
```
- **CSV input:** stops with ID, name, latitude, longitude, deadline.
- **Haversine matrix:** pairwise travel times (minutes) between every stop.
- **Floyd-Warshall:** fills the all-pairs shortest-path matrix.
- **Greedy schedule:** assigns stops to shuttles sorted by earliest deadline first.
- **Local search:** 2-opt swaps + inter-shuttle relocate to reduce total cost.
- **Export:** final schedule written to `output/final_schedule.csv`.

### Slide 4 — Technology Used
- **Language:** C++14 — no external libraries, standard library only.
- **STL used:** `std::vector`, `std::map`, `std::sort`, `std::reverse`.
- **Build system:** GNU `make` with a cross-platform `Makefile` (`#ifdef _WIN32` for directory creation).
- **Data format:** plain CSV for both input and output — no database required.

### Slide 5 — Implementation
**4 OOP classes:**

| Class | Defined in | Responsibility |
|---|---|---|
| `Config` | `include/shuttle.h` | Stores compile-time constants: `START_TIME`, `DWELL_TIME`, `MAX_ITERATIONS`, speed |
| `Stop` | `include/shuttle.h` | Stop data (id, name, lat, lon, deadline); `operator<` enables deadline-based sort |
| `ScheduleEntry` | `include/shuttle.h` | One stop visit: `stopId`, `arrivalTime`, `departureTime` |
| `ShuttleSchedule` | `include/shuttle.h` | One shuttle's full route: `shuttleId` + `vector<ScheduleEntry>` |

**5 source modules:** `main.cpp`, `data.cpp`, `scheduler.cpp`, `optimizer.cpp`, `evaluator.cpp`.

**Key algorithms:** Haversine (travel time), Floyd-Warshall O(n³) (shortest paths), Greedy O(n·s) (initial schedule), 2-opt O(n²) + Relocate O(n²·s²) (optimisation).

**C++ concepts demonstrated:** OOP, `operator<` and `operator<<` overloading, `friend` functions, `static` members, `try/catch` exception handling, `extern` global (`g_stopIdxMap`).

### Slide 6 — Results
| Metric | Baseline | Optimised |
|---|---|---|
| Stops on time | 4 / 4 (100%) | 4 / 4 (100%) |
| Total cost | 146 | 116 |
| Iterations to converge | — | 8 |

- Edge-case test with non-contiguous IDs (7, 10, 25, 42): **same correct output, no crash**.
- Error handling: missing CSV → exit code 1 with descriptive message; 0 shuttles → exit code 1.

### Slide 7 — Limitations
- **Fixed speed:** travel time assumes a constant average speed; real traffic is ignored.
- **Small dataset:** only 4 stops in Dehradun (Clock Tower, ISBT, Railway Station, Rajpur Road).
- **No GUI:** CLI only — no map visualisation.
- **No real-time data:** schedule is static; no live re-routing if a shuttle is delayed.

### Slide 8 — Future Scope
*(See [`FUTURE_SCOPE.md`](FUTURE_SCOPE.md) for full details.)*
- Real-time GPS integration and dynamic re-scheduling.
- Passenger demand and capacity constraints.
- Web or mobile front-end for operators.
- Larger city datasets and multi-vehicle routing problem (VRP) formulations.

### Slide 9 — Conclusion
ShuttleOpt demonstrates that a compact C++14 application using classical algorithms (Floyd-Warshall, greedy scheduling, local search) can automate shuttle assignment, achieve 100% on-time delivery, and reduce total travel/wait cost by 21% in our test scenario. The project integrates core data structures, OOP design, and real-world GPS data into a working command-line tool.

---

## SECTION B — Live Demo Script

> **Fallback:** If the binary fails to run, show `pre_captured_output.txt`. If the compiler is missing, open the source files in a text editor and walk through the code instead.

### Step 1 — Show Project Structure
```powershell
dir   # or: ls on Linux/macOS
```
**Point out:** `src/`, `data/`, `output/`, `docs/`, `Makefile`.  
**Concept:** organised modular project layout.

### Step 2 — Show the Input CSV
```powershell
type data\stops.csv
```
**Expected output:** 4 rows — ID, name, latitude, longitude, deadline.  
**Explain:** GPS coordinates feed Haversine; deadline drives greedy sort order.

### Step 3 — Normal Run
```powershell
.\shuttle_optimizer.exe data\stops.csv 2
```
**Walk through each output section:**
- `Loaded N stops` — CSV parsing succeeded.
- Travel time matrix — Haversine results in minutes.
- Floyd-Warshall matrix — all-pairs shortest paths.
- Baseline schedule — greedy assignment by earliest deadline.
- Optimisation log — iteration count, cost reduction (146 → 116).
- Final schedule — per-shuttle stop list with arrival times.
- Evaluation — on-time count, total travel time, total wait time.

### Step 4 — Show Exported CSV
```powershell
type output\final_schedule.csv
```
**Explain:** `evaluator.cpp` writes this; demonstrates file I/O and the `Evaluator` class.

### Step 5 — Edge Case (Non-Contiguous IDs)
```powershell
.\shuttle_optimizer.exe data\stops_edge.csv 2
```
**Expected:** same correct schedule output, no crash despite IDs 7, 10, 25, 42.  
**Explain:** `g_stopIdxMap` maps arbitrary stop IDs to contiguous matrix indices (0, 1, 2, 3). Without this, `travelTimes[stopId]` would index out-of-bounds.

### Step 6 — Error Handling
```powershell
.\shuttle_optimizer.exe data\missing.csv
```
**Expected:** descriptive error message, exit code 1.  
**Explain:** `try/catch` in `main.cpp` catches file-open failure and invalid shuttle count, prints a user-friendly message instead of crashing.

---

## SECTION C — Viva Questions & Answers

### Basic (Q1–Q6)

**Q1. What does this program do?**  
ShuttleOpt reads a CSV of transit stops (with GPS coordinates and deadlines), computes travel times using the Haversine formula, builds a shortest-path matrix with Floyd-Warshall, assigns stops to shuttles greedily, and then refines the schedule using local search to reduce total cost while keeping all stops on time.

**Q2. What is a shuttle deadline?**  
A deadline is the latest time (in minutes since midnight) by which a shuttle must arrive at a stop. Missing the deadline means a passenger is stranded. The greedy scheduler sorts stops by earliest deadline first (EDF) to maximise on-time arrivals.

**Q3. What is the Haversine formula?**  
Haversine computes the great-circle distance between two points on Earth given their latitudes and longitudes. We use it in `data.cpp` to calculate the straight-line distance (in km) between every pair of stops, then divide by an assumed average speed to get travel time in minutes.

**Q4. What is Floyd-Warshall?**  
Floyd-Warshall is an O(n³) dynamic programming algorithm that computes the shortest path between **all pairs** of nodes in a weighted graph. In our project it fills the `travelTimes[n][n]` matrix so that the scheduler always uses the optimal inter-stop travel time, even if a direct route is slower than going via a third stop.

**Q5. What is a greedy algorithm?**  
A greedy algorithm makes the locally optimal choice at each step without backtracking. Our scheduler sorts stops by deadline and assigns each stop to the shuttle that can reach it earliest. It is fast (O(n·s)) but may not produce a globally optimal schedule — which is why we follow up with local search.

**Q6. What OOP concepts does the project use?**  
The project uses four classes (`Stop`, `Shuttle`, `Schedule`, `Evaluator`), operator overloading (`Stop::operator<` for deadline-based sorting, `operator<<` for pretty-printing), friend functions (for `operator<<`), and static class members. Exception handling (`try/catch`) and `extern` globals (`g_stopIdxMap`) are also demonstrated.

---

### Intermediate (Q7–Q14)

**Q7. Why use Floyd-Warshall here instead of Dijkstra?**  
Floyd-Warshall computes **all-pairs** shortest paths in one O(n³) pass, which is ideal because the scheduler needs travel times between every pair of stops repeatedly. With only 4 stops n³ = 64 operations — negligible. Dijkstra would need to be run n times (once per source) for the same result.

**Q8. Explain operator overloading with a code example.**  
`Stop::operator<` is overloaded to compare two stops by their `deadline` field. This lets `std::sort(stops.begin(), stops.end())` automatically sort stops in deadline order without a custom comparator lambda. `operator<<` is overloaded as a `friend` function so `std::cout << stop` prints human-readable stop information.

**Q9. What is the role of `g_stopIdxMap`?**  
`g_stopIdxMap` is an `extern` global `std::map<int,int>` defined in `data.cpp` and used in `optimizer.cpp`. It maps an arbitrary stop ID (e.g., 42) to a contiguous matrix index (e.g., 3). This allows the travel-time matrix to be a compact `n×n` array regardless of how large or non-contiguous the stop IDs are.

**Q10. What is 2-opt?**  
2-opt is a local search move for route optimisation. It picks two edges in a shuttle's route and reverses the segment between them, checking whether the new route is shorter. In our code, 2-opt is applied per-shuttle to reduce each shuttle's individual route cost.

**Q11. How does the relocate phase work?**  
The relocate phase tries moving each stop from its current shuttle to every position in every other shuttle's route. If the move reduces total cost without making any stop late, the move is accepted. This is an inter-shuttle improvement that 2-opt (which is intra-shuttle) cannot achieve. Its complexity is O(n²·s²).

**Q12. What does `DWELL_TIME` represent?**  
`DWELL_TIME` is a constant (in minutes) representing how long a shuttle waits at a stop for passengers to board and alight. It is added to the shuttle's current time at each stop visit, ensuring that travel-time calculations account for realistic stop service time.

**Q13. How are times stored in the program?**  
All times are stored as **minutes since midnight** (integers or doubles). For example, a deadline of `480` means 8:00 AM. This makes arithmetic straightforward — adding travel time and dwell time is simple addition — and avoids date/time library complexity.

**Q14. What does `evaluateSchedule()` calculate?**  
`evaluateSchedule()` iterates over every stop in the final schedule and computes: (1) **on-time percentage** — the fraction of stops reached before their deadline; (2) **total travel time** — sum of all inter-stop travel durations; (3) **total wait time** — sum of idle time a shuttle spends waiting at stops. It also triggers the CSV export.

---

### Advanced (Q15–Q20)

**Q15. What happens if all stops have the same deadline?**  
The greedy sort by `operator<` is a stable sort by deadline, so all stops compare as equal. The assignment order then depends on the original CSV order. The schedule is still valid (no stop is artificially delayed), but the greedy phase provides no deadline-based ordering benefit. Local search still improves cost.

**Q16. What bug existed with direct stop-ID matrix indexing?**  
The original `scheduler.cpp` accessed the travel-time matrix as `travelTimes[lastStopId[s]][stop.id]`, using raw stop IDs as array indices. For non-contiguous IDs (e.g., 7, 10, 25, 42) this would index far outside the allocated `4×4` matrix, causing undefined behaviour or a crash. The fix was to look up the contiguous index via `g_stopIdxMap` before accessing the matrix.

**Q17. Why use local search instead of brute force?**  
Brute force over all stop permutations across s shuttles is factorial in complexity — completely infeasible even for modest inputs. Local search (2-opt + relocate) explores the neighbourhood of the current solution iteratively and converges to a good (not necessarily global optimum) solution in polynomial time. Our test showed convergence in 8 iterations.

**Q18. What is the overall time complexity of the optimizer?**  
- Floyd-Warshall: O(n³) — dominates the setup phase.  
- Greedy scheduling: O(n log n) for sort + O(n·s) for assignment.  
- Local search per iteration: O(n²) for 2-opt + O(n²·s²) for relocate.  
- Total across k iterations: O(k·n²·s²).  
For our dataset (n=4, s=2, k=8) this is trivially fast.

**Q19. How would you add passenger capacity constraints?**  
Each `Shuttle` object would gain a `capacity` field and a `currentLoad` counter. The greedy assignment step would check `currentLoad + stop.passengers <= capacity` before assigning. The relocate phase would similarly validate capacity on both the source and destination shuttle after a proposed move.

**Q20. What is the difference between baseline and optimised cost in the test run?**  
The baseline cost (146) is the total travel + wait time produced by the greedy schedule with no further improvement. After 8 iterations of 2-opt and inter-shuttle relocate, the optimised cost is 116 — a reduction of **~21%**. All 4 stops remain on time in both cases, so the improvement is purely in fleet efficiency, not at the expense of service quality.

---

## SECTION D — Suggested Team Division

> [!NOTE]
> This division is **SUGGESTED** based on module ownership. It does not represent verified individual contributions.

### Member 1 — Data & Infrastructure
**Owns:** `data.cpp`, `stops.csv`, Haversine implementation, Floyd-Warshall, `Makefile`.  
**Presents:** Slides 3–4 (pipeline and technology). Explains how GPS coordinates become a travel-time matrix and why Floyd-Warshall was chosen.  
**Viva ownership:** Q3 (Haversine), Q4 (Floyd-Warshall), Q7 (why Floyd-Warshall), Q13 (time storage).

### Member 2 — Scheduling
**Owns:** `scheduler.cpp`, `Stop::operator<`, greedy algorithm.  
**Presents:** Slides 1–2 (problem, motivation) and the greedy section of Slide 5.  
**Viva ownership:** Q1 (what the program does), Q2 (deadlines), Q5 (greedy), Q8 (operator overloading), Q15 (same-deadline edge case).

### Member 3 — Optimisation
**Owns:** `optimizer.cpp`, 2-opt, relocate, cost function, `g_stopIdxMap` fix.  
**Presents:** Optimisation portion of Slide 5 and Slide 6 (results).  
**Viva ownership:** Q9 (g_stopIdxMap), Q10 (2-opt), Q11 (relocate), Q16 (indexing bug), Q17 (local search vs brute force), Q18 (complexity), Q20 (baseline vs optimised).

### Member 4 — Evaluation & Presentation
**Owns:** `evaluator.cpp`, CSV export, all documentation, demo script.  
**Presents:** Slides 7–9 (limitations, future scope, conclusion) and runs the live demo.  
**Viva ownership:** Q6 (OOP concepts), Q12 (DWELL_TIME), Q14 (evaluateSchedule), Q19 (capacity constraints).

---

## SECTION E — Development Challenges

### Challenge 1 — `optimizeSchedule()` Was Never Called ✅ VERIFIED
The function was fully implemented in `optimizer.cpp` but was absent from `main.cpp` in the initial commit. The schedule was exported without any local-search refinement, so results were always baseline-only. **Fix:** wired the call into `main.cpp` between the greedy step and the evaluation step.

### Challenge 2 — Matrix Indexing with Raw Stop IDs ✅ VERIFIED
`travelTimes[lastStopId[s]][stop.id]` in the original `scheduler.cpp` used raw stop IDs as matrix indices. This works only if IDs are 0-based and contiguous. Non-contiguous IDs (e.g., 7, 10, 25, 42) caused out-of-bounds access. **Fix:** introduced `g_stopIdxMap` to translate any stop ID to a compact index (0…n-1) before matrix access. The edge-case test confirms this fix works correctly.

### Challenge 3 — Directory Creation via `system()` ✅ VERIFIED
The original `main.cpp` created the `output/` directory with `system("if not exist output mkdir output")`. This is Windows-only, fragile, and a security concern (shell injection). **Fix:** replaced with `_mkdir("output")` guarded by `#ifdef _WIN32` / `#else mkdir(...)` for cross-platform portability.

### Challenge 4 — Haversine Unit Handling ⚠️ PLAUSIBLE (not confirmed by Git history)
Dehradun stops are only a few kilometres apart. Errors in unit conversion (degrees → radians, km vs m) or in the Earth-radius constant produce travel-time values that are orders of magnitude wrong. It is plausible this required careful debugging, but no commit message or diff in the repository explicitly records a Haversine fix.
