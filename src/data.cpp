#include "shuttle.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>

using namespace std;

// ──────────────────────────────────────────────
// Global definitions
// ──────────────────────────────────────────────
vector<Stop>  g_allStops;
map<int, int> g_stopIdxMap; // stop ID -> matrix row/col index

// ──────────────────────────────────────────────
// Math helpers
// ──────────────────────────────────────────────
static const double EARTH_RADIUS_KM = 6371.0;
static const double PI = 3.14159265358979323846;

static double toRadians(double degrees) {
    return degrees * PI / 180.0;
}

static double haversineDistanceKm(double lat1, double lon1, double lat2, double lon2) {
    double dLat = toRadians(lat2 - lat1);
    double dLon = toRadians(lon2 - lon1);
    double a = sin(dLat / 2.0) * sin(dLat / 2.0) +
               cos(toRadians(lat1)) * cos(toRadians(lat2)) *
               sin(dLon / 2.0) * sin(dLon / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return EARTH_RADIUS_KM * c;
}

static void trimRight(string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' ||
                          s.back() == ' '  || s.back() == '\t')) {
        s.pop_back();
    }
}

// ──────────────────────────────────────────────
// Load stops from CSV and populate global maps
// ──────────────────────────────────────────────
vector<Stop> loadStops(const string& filename) {
    vector<Stop> stops;
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Cannot open stops file: " + filename);
    }

    string line;
    // Skip header line
    if (!getline(file, line)) {
        throw runtime_error("Stops file is empty: " + filename);
    }

    while (getline(file, line)) {
        trimRight(line);
        if (line.empty()) continue;

        istringstream iss(line);
        string token, stopName;

        getline(iss, token,    ','); trimRight(token);    int    id  = stoi(token);
        getline(iss, stopName, ','); trimRight(stopName);
        getline(iss, token,    ','); trimRight(token);    double lat = stod(token);
        getline(iss, token,    ','); trimRight(token);    double lon = stod(token);
        getline(iss, token);         trimRight(token);    int    req = stoi(token);

        stops.push_back(Stop(id, stopName, lat, lon, req));
    }

    // Build global state used by all modules
    g_allStops = stops;
    g_stopIdxMap.clear();
    for (int i = 0; i < static_cast<int>(stops.size()); i++) {
        g_stopIdxMap[stops[i].id] = i;
    }

    return stops;
}

// ──────────────────────────────────────────────
// Build N×N travel-time matrix (minutes).
// Row/col i corresponds to stops[i], accessed via g_stopIdxMap[stopId].
// Applies Floyd-Warshall for all-pairs shortest travel times.
// ──────────────────────────────────────────────
vector<vector<int>> buildTravelTimeMatrix(const vector<Stop>& stops) {
    int n = static_cast<int>(stops.size());
    vector<vector<int>> matrix(n, vector<int>(n, 0));

    // Direct travel times via Haversine
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                matrix[i][j] = 0;
            } else {
                double distKm = haversineDistanceKm(
                    stops[i].lat, stops[i].lon,
                    stops[j].lat, stops[j].lon);
                double timeMin = (distKm / Config::shuttleSpeed()) * 60.0;
                matrix[i][j] = max(1, static_cast<int>(round(timeMin)));
            }
        }
    }

    // Floyd-Warshall: find shortest path between every pair via intermediate stops
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (matrix[i][k] + matrix[k][j] < matrix[i][j]) {
                    matrix[i][j] = matrix[i][k] + matrix[k][j];
                }
            }
        }
    }

    return matrix;
}
