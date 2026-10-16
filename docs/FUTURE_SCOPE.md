# Future Scope — ShuttleOpt

This document outlines realistic future improvements to ShuttleOpt, organized by development effort. The current system handles 4 hardcoded Dehradun stops, 2 shuttles, CSV input, and a greedy + local-search optimizer (2-opt + relocate). It has no GUI, no passenger capacity, no real-time data, a fixed 25 km/h speed, and a fixed 7:00 AM start time.

---

## Short Term — 1–2 Weeks

Achievable within the existing codebase with minimal restructuring.

1. **Add a Makefile test target**
   - Add a `test` target to the Makefile that runs default and edge-case inputs and checks exit codes.
   - Example: `make test` runs `./ShuttleOpt stops.csv` and `./ShuttleOpt empty.csv` and asserts `$?` equals the expected value.
   - Requires no changes to C++ source — pure shell scripting inside the Makefile.

2. **Configurable speed and start time via CLI args**
   - `Config::shuttleSpeed()` and `Config::START_TIME` are currently compile-time constants.
   - Add `--speed 30` and `--start 08:00` flags parsed from `argv`.
   - Pass the parsed values into `Config` or directly as function arguments to `generateBaselineSchedule()` and `recomputeTimes()`.

3. **Better deadline miss reporting**
   - Currently late stops print `** LATE **` inline.
   - Add a summary block at the end of `evaluateSchedule()` showing the count of missed deadlines and total accumulated lateness in minutes.
   - Small addition to the existing evaluation loop — no structural changes needed.

4. **Multiple output formats**
   - Add an `--output-format json` CLI flag.
   - Implement `exportScheduleToJSON()` in `evaluator.cpp` using standard `std::string` operations (no external library needed).
   - Outputs a JSON file alongside or instead of the CSV.

---

## Medium Term — 1–2 Months

Requires new functionality or moderate restructuring of the existing classes.

1. **Passenger capacity constraints**
   - Add an `int demand` field to the `Stop` struct and an `int capacity` / `int currentLoad` pair to `ShuttleSchedule`.
   - Modify `generateBaselineSchedule()` to skip full shuttles and `recomputeTimes()` to track load.
   - Update `computeTotalCost()` with an overload penalty term.
   - Add a `--capacity N` CLI argument.
   - Read a `demand` column from `stops.csv`.

2. **Multi-depot support**
   - Currently all shuttles implicitly depart from the same origin at `START_TIME`.
   - Introduce a `Depot` class with coordinates, assign one per shuttle.
   - Modify `generateBaselineSchedule()` to use per-shuttle start coordinates when computing the first travel time leg.

3. **Dynamic stop loading from multiple CSV files**
   - Accept multiple route filenames on the command line.
   - Modify `loadStops()` to iterate over filenames and merge the resulting `std::vector<Stop>` collections.
   - Allows compositing routes without changing the optimizer.

4. **Simple text-based route visualization**
   - Print an ASCII timeline or route map showing which stops each shuttle visits and in what order.
   - New function in `evaluator.cpp` using `std::string` padding and alignment — no external dependency.

---

## Long Term — 3–6+ Months

Substantial new development beyond the current architecture.

1. **Graphical user interface**
   - A Qt or SFML window showing a map of Dehradun with stop markers and animated shuttle routes.
   - Users could drag stops, edit deadlines interactively, and watch the schedule update in real time.

2. **Real-time GPS integration**
   - Read a live GPS feed (file poll or serial port) to track actual shuttle positions.
   - Recalculate remaining stop assignments dynamically as shuttles deviate from plan.

3. **Larger-scale city data**
   - Import stops and road networks from OpenStreetMap or GTFS transit feed format.
   - Requires a proper graph data structure (adjacency list), Dijkstra or A* for shortest-path routing, and scaling to hundreds of stops.

---

## Ranked Top 3 Improvements by Feasibility × Impact

### 1. Passenger Capacity Constraints *(Highest Impact, Medium Feasibility)*

**Problem it solves:** The current optimizer assigns stops to shuttles without regard to how many passengers each stop generates. In practice, an overloaded shuttle is operationally infeasible — this is the single most important real-world limitation of the current model.

**What to implement:**
- `demand` per stop, `capacity` and `currentLoad` per shuttle.
- Baseline generation skips a full shuttle; cost function penalizes overload.

**Step-by-step implementation plan:**

1. Add `int demand` to `Stop` in `shuttle.h`:
   ```cpp
   struct Stop {
       std::string name;
       double lat, lon;
       int deadline;
       int demand;   // ← new
   };
   ```

2. Add capacity fields to `ShuttleSchedule` in `shuttle.h`:
   ```cpp
   struct ShuttleSchedule {
       std::vector<Stop> stops;
       int capacity = 40;     // ← new
       int currentLoad = 0;   // ← new
   };
   ```

3. In `generateBaselineSchedule()`, skip assignment when full:
   ```cpp
   if (shuttle.currentLoad + stop.demand > shuttle.capacity) continue;
   shuttle.currentLoad += stop.demand;
   ```

4. In `recomputeTimes()`, accumulate and track load per stop visit.

5. In `computeTotalCost()`, add an overload penalty:
   ```cpp
   if (shuttle.currentLoad > shuttle.capacity)
       cost += OVERLOAD_PENALTY * (shuttle.currentLoad - shuttle.capacity);
   ```

6. Parse `--capacity N` from `argv` and apply to all (or per-) shuttles.

7. Add a `demand` column to `stops.csv` and update `loadStops()` to read it.

**Trade-off:** The greedy baseline may now leave some stops unassigned if all shuttles are at capacity — need a fallback or a warning. Local search moves (2-opt / relocate) must also check capacity before accepting a swap.

---

### 2. Configurable Speed and Start Time *(High Feasibility, Medium Impact)*

**Problem it solves:** Hard-coded 25 km/h and 7:00 AM make the tool useless for any scenario outside the original demo. Fixing this takes roughly 30 minutes and immediately makes ShuttleOpt a general-purpose tool.

**What to implement:**
- Parse `--speed <kmh>` and `--start <HH:MM>` from `argv` in `main.cpp`.
- Pass values into `Config` setters or directly as parameters to `generateBaselineSchedule()` and `recomputeTimes()`.

**Steps:**
1. Add `--speed` and `--start` to the `argv` parsing block in `main.cpp`.
2. Convert `HH:MM` string to minutes-since-midnight integer.
3. Replace `Config::shuttleSpeed()` and `Config::START_TIME` uses with the parsed values.

**Trade-off:** Minimal. Only risk is malformed input — add basic validation with a clear usage message.

---

### 3. Better Deadline Miss Reporting *(Highest Feasibility, Meaningful Demo Impact)*

**Problem it solves:** Inline `** LATE **` markers are easy to miss in long output. A summary makes missed deadlines immediately visible and strengthens the evaluation output for demos and testing.

**What to implement:**
- At the end of `evaluateSchedule()`, print a summary:
  ```
  --- Deadline Summary ---
  Missed: 1 stop(s), total lateness: 4 min
  ```

**Steps:**
1. Add `int missedCount = 0` and `int totalLateness = 0` accumulators to `evaluateSchedule()`.
2. Inside the existing per-stop loop, increment both when a stop is late.
3. Print the summary block after the per-shuttle output.

**Trade-off:** None significant — this is a pure additive change to an existing function with no risk of regression.
