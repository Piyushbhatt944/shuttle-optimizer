#include "shuttle.h"
#include <algorithm>
#include <climits>

using namespace std;

// Greedy scheduler: assigns stops to shuttles in deadline order.
// Each stop is given to the shuttle that can arrive earliest.
// Uses g_stopIdxMap to safely index the travel-time matrix by stop ID.
vector<ShuttleSchedule> generateBaselineSchedule(
    const vector<Stop>&        stops,
    const vector<vector<int>>& travelTimes,
    int                        numShuttles)
{
    // Sort stops by ascending deadline (earliest deadline first)
    vector<Stop> sortedStops = stops;
    sort(sortedStops.begin(), sortedStops.end());

    vector<ShuttleSchedule> schedules(numShuttles);
    vector<int> lastStopId(numShuttles, -1);           // last stop ID served
    vector<int> currentTime(numShuttles, Config::START_TIME); // shuttle clock

    for (int i = 0; i < numShuttles; i++) {
        schedules[i] = ShuttleSchedule(i);
    }

    for (const auto& stop : sortedStops) {
        int stopIdx    = g_stopIdxMap.at(stop.id); // safe matrix index for this stop
        int bestShuttle = -1;
        int bestArrival = INT_MAX;

        for (int s = 0; s < numShuttles; s++) {
            int arrival;
            if (lastStopId[s] == -1) {
                // Shuttle hasn't moved yet — it starts at START_TIME
                arrival = Config::START_TIME;
            } else {
                int lastIdx = g_stopIdxMap.at(lastStopId[s]); // safe index
                arrival = currentTime[s] + travelTimes[lastIdx][stopIdx];
            }

            if (arrival < bestArrival) {
                bestArrival = arrival;
                bestShuttle = s;
            }
        }

        schedules[bestShuttle].stops.push_back(
            ScheduleEntry(stop.id, bestArrival, bestArrival + Config::DWELL_TIME));

        lastStopId[bestShuttle]  = stop.id;
        currentTime[bestShuttle] = bestArrival + Config::DWELL_TIME;
    }

    return schedules;
}
