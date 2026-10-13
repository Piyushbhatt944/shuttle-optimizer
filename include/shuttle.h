#ifndef SHUTTLE_H
#define SHUTTLE_H
#include <vector>
#include <string>
#include <iostream>
using namespace std;

class Config {
public:
    enum { START_TIME = 420, DWELL_TIME = 2, MAX_ITERATIONS = 500 };
    static double shuttleSpeed() { return 25.0; }
};

class Stop {
public:
    int id;
    string name;
    double lat, lon;
    int requiredByTime;

    Stop() : id(0), name(""), lat(0), lon(0), requiredByTime(0) {}

    Stop(int id, string name, double lat, double lon, int requiredByTime)
        : id(id), name(name), lat(lat), lon(lon), requiredByTime(requiredByTime) {}

    bool operator<(const Stop& other) const {
        return requiredByTime < other.requiredByTime;
    }

    friend ostream& operator<<(ostream& os, const Stop& s) {
        os << "[" << s.id << "] " << s.name
           << " (" << s.lat << ", " << s.lon << ")"
           << " deadline: " << s.requiredByTime;
        return os;
    }
};

class ScheduleEntry {
public:
    int stopId;
    int arrivalTime;
    int departureTime;

    ScheduleEntry() : stopId(0), arrivalTime(0), departureTime(0) {}

    ScheduleEntry(int stopId, int arrivalTime, int departureTime)
        : stopId(stopId), arrivalTime(arrivalTime), departureTime(departureTime) {}

    friend ostream& operator<<(ostream& os, const ScheduleEntry& e) {
        os << "Stop " << e.stopId
           << " | arrive: " << e.arrivalTime
           << " | depart: " << e.departureTime;
        return os;
    }
};

class ShuttleSchedule {
public:
    int shuttleId;
    vector<ScheduleEntry> stops;

    ShuttleSchedule() : shuttleId(0) {}

    ShuttleSchedule(int shuttleId) : shuttleId(shuttleId) {}

    ~ShuttleSchedule() { stops.clear(); }

    friend ostream& operator<<(ostream& os, const ShuttleSchedule& s) {
        os << "Shuttle " << s.shuttleId << " (" << s.stops.size() << " stops)";
        return os;
    }
};

vector<Stop> loadStops(const string& filename);
vector<vector<int>> buildTravelTimeMatrix(const vector<Stop>& stops);

vector<ShuttleSchedule> generateBaselineSchedule(
    const vector<Stop>& stops,
    const vector<vector<int>>& travelTimes,
    int numShuttles);

vector<ShuttleSchedule> optimizeSchedule(
    vector<ShuttleSchedule> baseline,
    const vector<vector<int>>& travelTimes);

void evaluateSchedule(const vector<ShuttleSchedule>& schedule, const string& label);
void exportScheduleToCSV(const vector<ShuttleSchedule>& schedule, const string& filename);

#endif
