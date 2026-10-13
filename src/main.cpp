#include "shuttle.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <cstdlib>
using namespace std;

extern vector<Stop> g_allStops;

int main(int argc, char* argv[]) {
    string stopsFile  = "data/stops.csv";
    int numShuttles   = 2;

    if (argc >= 2) stopsFile   = argv[1];
    if (argc >= 3) numShuttles = stoi(argv[2]);

    cout << "============================================================\n";
    cout << "  ShuttleOpt\n";
    cout << "============================================================\n";
    cout << "Stops file:      " << stopsFile << "\n";
    cout << "Shuttle count:   " << numShuttles << "\n\n";

    vector<Stop> stops;
    try {
        stops = loadStops(stopsFile);
    } catch (const exception& e) {
        cerr << "Error loading stops: " << e.what() << "\n";
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

#ifdef _WIN32
    system("if not exist output mkdir output");
#else
    system("mkdir -p output");
#endif
    exportScheduleToCSV(baseline, "output/final_schedule.csv");

    cout << "\nDone.\n";
    return 0;
}
