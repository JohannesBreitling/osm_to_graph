
#include <cstdint>

#pragma once

struct Bounds {
    float minlat;
    float maxlat;
    float minlong;
    float maxlong;
};

struct Node {
    int64_t id;
    float latitude;
    float longitude;
};

struct Edge {
    int64_t wayId;
    uint32_t maxSpeed;
    int64_t from;
    int64_t to;
};

struct Way {
    int64_t id;
    uint32_t maxSpeed;
    bool oneway;
    std::string streetName;
    std::string type;
    std::vector<int64_t> nodes;
};

struct GraphEdge {
    uint32_t from; // Indices in the graph
    uint32_t to;
    int64_t osmWayId;
    uint32_t geoLength; // In m
    uint32_t travelTime; // In ms
};

struct Building {
    std::vector<int64_t> nodes;
    std::string housenumber;
    std::string street;
    std::string postcode;
    std::string city;
};