#include "shuttle.h"
#include <algorithm>
#include <iostream>

using namespace std;

static int getRequiredByTime(int stopId) {
    for (const auto& s : g_allStops) {
        if (s.id == stopId) return s.requiredByTime;
    }
    return 0;
}

static bool recomputeTimes(ShuttleSchedule&           schedule,
                           const vector<vector<int>>& travelTimes) {
    for (size_t i = 0; i < schedule.stops.size(); i++) {
        int arrival;
        if (i == 0) {
            arrival = Config::START_TIME;
        } else {
            int prevIdx = g_stopIdxMap.at(schedule.stops[i - 1].stopId);
            int currIdx = g_stopIdxMap.at(schedule.stops[i].stopId);
            arrival = schedule.stops[i - 1].departureTime +
                      travelTimes[prevIdx][currIdx];
        }
        schedule.stops[i].arrivalTime   = arrival;
        schedule.stops[i].departureTime = arrival + Config::DWELL_TIME;

        if (arrival > getRequiredByTime(schedule.stops[i].stopId)) {
            return false;
        }
    }
    return true;
}

static int computeTotalCost(const vector<ShuttleSchedule>& schedules,
                            const vector<vector<int>>&      travelTimes) {
    int totalTravel = 0;
    int totalWait   = 0;

    for (const auto& sched : schedules) {
        for (size_t i = 1; i < sched.stops.size(); i++) {
            int prevIdx = g_stopIdxMap.at(sched.stops[i - 1].stopId);
            int currIdx = g_stopIdxMap.at(sched.stops[i].stopId);
            totalTravel += travelTimes[prevIdx][currIdx];
        }
        for (const auto& entry : sched.stops) {
            int slack = getRequiredByTime(entry.stopId) - entry.arrivalTime;
            if (slack > 0) totalWait += slack;
        }
    }

    return totalTravel + totalWait;
}

vector<ShuttleSchedule> optimizeSchedule(
    vector<ShuttleSchedule>    baseline,
    const vector<vector<int>>& travelTimes)
{
    vector<ShuttleSchedule> best = baseline;
    int bestCost    = computeTotalCost(best, travelTimes);
    int originalCost = bestCost;

    bool improved  = true;
    int  iterations = 0;

    while (improved && iterations < Config::MAX_ITERATIONS) {
        improved = false;
        iterations++;

        for (size_t s = 0; s < best.size() && !improved; s++) {
            if (best[s].stops.size() < 2) continue;

            for (size_t i = 0; i < best[s].stops.size() - 1 && !improved; i++) {
                for (size_t j = i + 1; j < best[s].stops.size() && !improved; j++) {
                    vector<ShuttleSchedule> candidate = best;

                    reverse(candidate[s].stops.begin() + i,
                            candidate[s].stops.begin() + j + 1);

                    if (recomputeTimes(candidate[s], travelTimes)) {
                        int candidateCost = computeTotalCost(candidate, travelTimes);
                        if (candidateCost < bestCost) {
                            best      = candidate;
                            bestCost  = candidateCost;
                            improved  = true;
                        }
                    }
                }
            }
        }

        if (!improved) {
            for (size_t s1 = 0; s1 < best.size() && !improved; s1++) {
                for (size_t s2 = 0; s2 < best.size() && !improved; s2++) {
                    if (s1 == s2) continue;

                    for (size_t i = 0; i < best[s1].stops.size() && !improved; i++) {
                        for (size_t j = 0; j <= best[s2].stops.size() && !improved; j++) {
                            vector<ShuttleSchedule> candidate = best;

                            ScheduleEntry moving = candidate[s1].stops[i];
                            candidate[s1].stops.erase(candidate[s1].stops.begin() + i);
                            candidate[s2].stops.insert(candidate[s2].stops.begin() + j, moving);

                            bool feasible = true;
                            if (!candidate[s1].stops.empty()) {
                                feasible = recomputeTimes(candidate[s1], travelTimes);
                            }
                            if (feasible) {
                                feasible = recomputeTimes(candidate[s2], travelTimes);
                            }

                            if (feasible) {
                                int candidateCost = computeTotalCost(candidate, travelTimes);
                                if (candidateCost < bestCost) {
                                    best      = candidate;
                                    bestCost  = candidateCost;
                                    improved  = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    cout << "Optimisation: " << iterations << " iteration(s), cost "
         << originalCost << " -> " << bestCost << "\n";
    return best;
}
