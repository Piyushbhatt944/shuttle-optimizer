#include "shuttle.h"
#include <algorithm>
#include <climits>

using namespace std;

vector<ShuttleSchedule> generateBaselineSchedule(
    const vector<Stop>&        stops,
    const vector<vector<int>>& travelTimes,
    int                        numShuttles)
{
    vector<Stop> sortedStops = stops;
    sort(sortedStops.begin(), sortedStops.end());

    vector<ShuttleSchedule> schedules(numShuttles);
    vector<int> lastStopId(numShuttles, -1);
    vector<int> currentTime(numShuttles, Config::START_TIME);

    for (int i = 0; i < numShuttles; i++) {
        schedules[i] = ShuttleSchedule(i);
    }

    for (const auto& stop : sortedStops) {
        int stopIdx    = g_stopIdxMap.at(stop.id);
        int bestShuttle = -1;
        int bestArrival = INT_MAX;

        for (int s = 0; s < numShuttles; s++) {
            int arrival;
            if (lastStopId[s] == -1) {
                arrival = Config::START_TIME;
            } else {
                int lastIdx = g_stopIdxMap.at(lastStopId[s]);
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
