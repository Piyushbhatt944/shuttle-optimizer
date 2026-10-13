#include "shuttle.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>
using namespace std;

extern vector<Stop> g_allStops;

static string minutesToHHMM(int minutes) {
    int h = minutes / 60;
    int m = minutes % 60;
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    return string(buf);
}

static string getStopName(int stopId) {
    for (const auto& stop : g_allStops) {
        if (stop.id == stopId) return stop.name;
    }
    return "Unknown (ID " + to_string(stopId) + ")";
}

static int getRequiredByTime(int stopId) {
    for (const auto& stop : g_allStops) {
        if (stop.id == stopId) return stop.requiredByTime;
    }
    return 0;
}

void evaluateSchedule(const vector<ShuttleSchedule>& schedule,
                      const string& label) {
    cout << "\n" << string(60, '=') << "\n";
    cout << "  Schedule: " << label << "\n";
    cout << string(60, '=') << "\n";

    int totalTravel  = 0;
    int totalWait    = 0;
    int totalStops   = 0;
    int onTimeStops  = 0;

    for (const auto& shuttle : schedule) {
        cout << "\n--- Shuttle " << shuttle.shuttleId
             << " (" << shuttle.stops.size() << " stops) ---\n";
        cout << left
             << setw(28) << "Stop"
             << setw(10) << "Arrival"
             << setw(12) << "Departure"
             << setw(12) << "Deadline"
             << "Status\n";
        cout << string(68, '-') << "\n";

        for (size_t i = 0; i < shuttle.stops.size(); i++) {
            const auto& entry = shuttle.stops[i];
            int requiredBy = getRequiredByTime(entry.stopId);
            bool onTime = (entry.arrivalTime <= requiredBy);

            cout << left
                 << setw(28) << getStopName(entry.stopId)
                 << setw(10) << minutesToHHMM(entry.arrivalTime)
                 << setw(12) << minutesToHHMM(entry.departureTime)
                 << setw(12) << minutesToHHMM(requiredBy)
                 << (onTime ? "ON TIME" : "** LATE **") << "\n";

            totalStops++;
            if (onTime) onTimeStops++;

            int slack = requiredBy - entry.arrivalTime;
            if (slack > 0) totalWait += slack;

            if (i > 0) {
                int travel = entry.arrivalTime - shuttle.stops[i - 1].departureTime;
                totalTravel += travel;
            }
        }
    }

    cout << "\n--- Metrics ---\n";
    cout << "Total travel time:     " << totalTravel << " minutes\n";
    cout << "Total wait/slack time: " << totalWait << " minutes\n";
    cout << "Total combined cost:   " << (totalTravel + totalWait) << " minutes\n";
    cout << "On-time performance:   " << onTimeStops << "/" << totalStops
         << " (" << fixed << setprecision(1)
         << (totalStops > 0 ? (100.0 * onTimeStops / totalStops) : 0.0)
         << "%)\n";
}

void exportScheduleToCSV(const vector<ShuttleSchedule>& schedule,
                         const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot open output file: " << filename << "\n";
        return;
    }

    file << "shuttle_id,stop_id,stop_name,arrival_time,departure_time\n";

    for (const auto& shuttle : schedule) {
        for (const auto& entry : shuttle.stops) {
            file << shuttle.shuttleId << ","
                 << entry.stopId << ","
                 << getStopName(entry.stopId) << ","
                 << minutesToHHMM(entry.arrivalTime) << ","
                 << minutesToHHMM(entry.departureTime) << "\n";
        }
    }

    file.close();
    cout << "\nSchedule exported to: " << filename << "\n";
}
