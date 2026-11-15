#include "shuttle.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

using namespace std;

Config g_config;

static void ensureOutputDir(const string& path) {
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

static void printUsage(const char* prog) {
    cout << "Usage: " << prog << " [stops.csv] [shuttles] [options]\n\n"
         << "Options:\n"
         << "  --speed <km/h>    Shuttle speed (default: 25.0)\n"
         << "  --start <HH:MM>  Departure time (default: 07:00)\n"
         << "  --dwell <mins>    Dwell time per stop (default: 2)\n"
         << "  --help            Show this help message\n";
}

static int parseHHMM(const string& s) {
    size_t pos = s.find(':');
    if (pos == string::npos) return stoi(s) * 60;
    int h = stoi(s.substr(0, pos));
    int m = stoi(s.substr(pos + 1));
    return h * 60 + m;
}

int main(int argc, char* argv[]) {
    string stopsFile = "data/stops.csv";
    int numShuttles  = 2;

    int i = 1;
    while (i < argc) {
        string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--speed" && i + 1 < argc) {
            g_config.speed = stod(argv[++i]);
        } else if (arg == "--start" && i + 1 < argc) {
            g_config.startTime = parseHHMM(argv[++i]);
        } else if (arg == "--dwell" && i + 1 < argc) {
            g_config.dwellTime = stoi(argv[++i]);
        } else if (arg[0] != '-' && stopsFile == "data/stops.csv" && i == 1) {
            stopsFile = arg;
        } else if (arg[0] != '-') {
            try {
                numShuttles = stoi(arg);
            } catch (const exception&) {
                cerr << "Error: invalid argument '" << arg << "'. Use --help for usage.\n";
                return 1;
            }
        } else {
            cerr << "Error: unknown option '" << arg << "'. Use --help for usage.\n";
            return 1;
        }
        i++;
    }

    if (numShuttles < 1) {
        cerr << "Error: shuttle count must be at least 1.\n";
        return 1;
    }

    cout << "============================================================\n";
    cout << "  ShuttleOpt\n";
    cout << "============================================================\n";
    cout << "Stops file:      " << stopsFile                        << "\n";
    cout << "Shuttle count:   " << numShuttles                      << "\n";
    cout << "Speed:           " << g_config.speed   << " km/h"     << "\n";
    cout << "Start time:      ";
    printf("%02d:%02d\n", g_config.startTime / 60, g_config.startTime % 60);
    cout << "Dwell time:      " << g_config.dwellTime << " min"    << "\n\n";

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
