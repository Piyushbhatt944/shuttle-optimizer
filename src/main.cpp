#include "shuttle.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

using namespace std;

extern vector<Stop> g_allStops;

static void ensureOutputDir(const string& path) {
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

int main(int argc, char* argv[]) {
    string stopsFile = "data/stops.csv";
    int numShuttles  = 2;

    if (argc >= 2) stopsFile = argv[1];
    if (argc >= 3) {
        try {
            numShuttles = stoi(argv[2]);
        } catch (const exception&) {
            cerr << "Error: shuttle count must be an integer.\n";
            return 1;
        }
    }

    if (numShuttles < 1) {
        cerr << "Error: shuttle count must be at least 1.\n";
        return 1;
    }

    cout << "============================================================\n";
    cout << "  ShuttleOpt\n";
    cout << "============================================================\n";
    cout << "Stops file:      " << stopsFile    << "\n";
    cout << "Shuttle count:   " << numShuttles  << "\n\n";

    vector<Stop> stops;
    try {
        stops = loadStops(stopsFile);
    } catch (const exception& e) {
        cerr << "Error loading stops: " << e.what() << "\n";
        return 1;
    }

    if (stops.empty()) {
        cerr << "Error: stops file contains no valid stops.\n";
        return 1;
    }

    cout << "Loaded " << stops.size() << " stops.\n";
    for (const auto& s : stops) {
        int h = s.requiredByTime / 60, m = s.requiredByTime % 60;
        printf("  [%d] %-28s (%.4f, %.4f)  deadline %02d:%02d\n",
               s.id, s.name.c_str(), s.lat, s.lon, h, m);
    }

    auto travelTimes = buildTravelTimeMatrix(stops);

    cout << "\nGenerating baseline schedule (greedy)...\n";
    auto baseline = generateBaselineSchedule(stops, travelTimes, numShuttles);
    evaluateSchedule(baseline, "Baseline (Greedy)");

    cout << "\nRunning local-search optimiser...\n";
    auto optimized = optimizeSchedule(baseline, travelTimes);
    evaluateSchedule(optimized, "Optimized (Local Search)");

    ensureOutputDir("output");
    exportScheduleToCSV(optimized, "output/final_schedule.csv");

    cout << "\nDone.\n";
    return 0;
}
