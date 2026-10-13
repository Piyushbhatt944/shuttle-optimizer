ShuttleOpt - Static Shuttle Schedule Optimizer for Urban Transit

We have 4 stops in Dehradun (Clock Tower, ISBT, Railway Station, Rajpur Road) and 2 shuttles. The program reads the stop data from a csv file, calculates how long it takes to travel between each pair of stops using the Haversine formula, and then builds a schedule using a greedy algorithm.

At the end it prints the schedule to the terminal and saves it as a csv file in the output folder.

How to compile (type this in the VS Code terminal):

    g++ -Iinclude -Wall -std=c++14 src/main.cpp src/data.cpp src/scheduler.cpp src/optimizer.cpp src/evaluator.cpp -o shuttle_optimizer

How to run (type this after compiling):

    

You can also pass arguments like this:

    .\shuttle_optimizer.exe data\stops.csv 2

DSA AND OOP CONCEPTS

DSA concepts used:

    - Graph representation using adjacency matrix (the travel time matrix)
    - Floyd-Warshall algorithm for all-pairs shortest path (data.cpp)
    - Greedy algorithm for schedule generation (scheduler.cpp)
    - Sorting using STL sort with operator overloading (scheduler.cpp)
    - 2D vectors as matrix data structure (data.cpp)
    - Linear search for stop lookup (evaluator.cpp, optimizer.cpp)

OOP concepts used:

    - Classes with public members (Stop, ScheduleEntry, ShuttleSchedule, Config)
    - Default constructors (Stop(), ScheduleEntry(), ShuttleSchedule())
    - Parameterized constructors (Stop(id, name, ...), ScheduleEntry(id, arr, dep))
    - Destructor (~ShuttleSchedule clears the stops vector)
    - Operator overloading (< for Stop comparison, << for printing)
    - Friend functions (operator<< declared as friend in each class)
    - Static class members (Config class with enum constants and static method)
    - Encapsulation (static helper functions visible only within their own file)
    - Modular design (4 separate modules with one shared header contract)

Some assumptions we made:

    - shuttle speed is 25 km/h which is reasonable for dehradun city traffic
    - each shuttle spends 2 minutes at every stop for boarding
    - all shuttles start at 7:00 AM
    - the coordinates are approximate real locations in dehradun

.\shuttle_optimizer.exe