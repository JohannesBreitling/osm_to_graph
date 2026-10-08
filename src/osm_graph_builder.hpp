
#define _USE_MATH_DEFINES

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <cassert>
#include <cmath>

#include "../external/pugixml/pugixml.hpp"

#include "./vector_util.hpp"

#include "./data/data.hpp"
#include "./data/constants.hpp"

#pragma once

struct OSMGraph {
    Bounds bounds;
    
    std::vector<uint32_t> firstOut;
    std::vector<uint32_t> heads;

    // Vertex attributes
    std::vector<float> lats;
    std::vector<float> longs;
    std::vector<int64_t> osmNodeIds;

    // Edge attributes
    std::vector<uint32_t> travelTimes;
    std::vector<uint32_t> geoDistances;
    std::vector<int64_t> osmWayIds;

    // Adress list
    std::vector<std::string> addressList;
};

void write_osm_graph_to_file(const OSMGraph &g, std::string path) {
    write_vector_to_file<uint32_t>(g.firstOut, path + "/first_out");
    write_vector_to_file<uint32_t>(g.heads, path + "/heads");
    write_vector_to_file<uint32_t>(g.geoDistances, path + "/geo_distances");
    write_vector_to_file<uint32_t>(g.travelTimes, path + "/travel_times");
    write_vector_to_file<int64_t>(g.osmNodeIds, path + "/osm_node_ids");
    write_vector_to_file<int64_t>(g.osmWayIds, path + "/osm_way_ids");
    write_vector_to_file<float>(g.lats, path + "/latitudes");
    write_vector_to_file<float>(g.longs, path + "/longitudes");

    write_string_list_to_file(g.addressList, path + "/address_list.txt");
}

class OSMGraphBuilder {

public:

    OSMGraphBuilder() {}

    void setBounds(const Bounds bounds_) {
        bounds = bounds_;
    }

    void addNode(const Node node) {        
        if (nodeIds.count(node.id))
            return;
        
        nodes.push_back(node);
        nodeIds.insert(node.id);
    }

    void addWay(const Way way) {
        ways.push_back(way);
    }

    void addBuilding(const Building building) {
        buildings.push_back(building);
    }

    OSMGraph finalize() {
        processWays();
        processBuildings();
        processNodes();
        processEdges();

        //* Future: Eliminate embedding vertices (indegree == 2, outdegree == 2)
        //* Create an overlay graph for the routing, but keep the original vertex information
        //* For path unpacking and visualization
 
        createStreetDictionary();

        return buildGraph();   
    }


private:

    uint32_t distanceBetweenNodes(const float lat1_, const float lon1, const float lat2_, const float lon2) const {
        using namespace std;
        const auto latDelta = (lat1_ - lat2_) * M_PI / 180;
        const auto longDelta = (lon1 - lon2) * M_PI / 180;
        const auto lat1 = lat1_ * M_PI / 180;
        const auto lat2 = lat2_ * M_PI / 180;
        
        double a = sin(latDelta / 2) * sin(latDelta / 2) + cos(lat1) * cos(lat2) * sin(longDelta / 2) * sin(longDelta / 2);
        double c = 2 * atan2(sqrt(a), sqrt(1 - a));
        
        return earth_radius * c;
    }

    uint32_t distanceBetweenNodes(const Node &n1, const Node &n2) const {
        return distanceBetweenNodes(n1.latitude, n1.longitude, n2.latitude, n2.longitude);
    }
    
    uint32_t travelTime(const uint32_t length, const uint32_t maxSpeed) const {
        const double ratio = (length + 0.0) / maxSpeed;
        return 3600 * ratio;
    }

    void processWays() {
        for (const auto &way : ways) {
            int64_t lastId = invalid_id;
            for (const auto nodeId : way.nodes) {
                refedNodeIds.insert(nodeId);

                if (lastId != invalid_id) {
                    Edge e;
                    e.from = lastId;
                    e.to = nodeId;
                    e.wayId = way.id;
                    e.maxSpeed = way.maxSpeed;
                    edges.push_back(e);

                    // Skip the back edge if the way is a oneway road
                    if (!way.oneway) {
                        Edge e2;
                        e2.to = lastId;
                        e2.from = nodeId;
                        e2.wayId = way.id;
                        e2.maxSpeed = way.maxSpeed;
                        edges.push_back(e2);
                    }
                }

                lastId = nodeId;
            }

            if (way.streetName == "")
                continue;

            for (const auto nodeId : way.nodes) {
                auto &vec = streetNameToStreetNodes[way.streetName];
                vec.push_back(nodeId);
            }
        }
    }

    void processNodes() {
        refedNodes = std::vector<Node>(refedNodeIds.size());
        uint32_t i = 0;
        for (const auto &node : nodes) {
            assert(node.id != 0);
            if (!refedNodeIds.count(node.id))
                continue;

            refedNodes[i] = node;
            osmIdToRefedNodeIdx[node.id] = i;
            ++i;
        }

        num_vertices = i;

        assert(i == refedNodes.size());

        i = 0;
        refedBuildingNodes = std::vector<Node>(refedBuildingNodeIds.size());
        for (const auto &node : nodes) {
            assert(node.id != 0);
            if (!refedBuildingNodeIds.count(node.id))
                continue;

            refedBuildingNodes[i] = node;
            osmIdToRefedBuildingNodeIdx[node.id] = i;
            ++i;
        }
    }

    void processEdges() {
        for (const auto &edge : edges) {
            const auto fromIdx = osmIdToRefedNodeIdx[edge.from];
            const auto toIdx = osmIdToRefedNodeIdx[edge.to];

            const auto &fromNode = refedNodes[fromIdx];
            const auto &toNode = refedNodes[toIdx];

            const auto l = distanceBetweenNodes(fromNode, toNode);
            const auto tt = travelTime(l, edge.maxSpeed);
            
            GraphEdge e;
            e.from = fromIdx;
            e.to = toIdx;
            e.osmWayId = edge.wayId;
            e.geoLength = l;
            e.travelTime = tt;

            graphEdges.push_back(e);
        }
    }

    void processBuildings() {
        for (const auto b : buildings) {
            for (const auto nodeId : b.nodes) {
                refedBuildingNodeIds.insert(nodeId);
            }
        }
    }

    void createStreetDictionary() {
        for (const auto b : buildings) {
            if (!streetNameToStreetNodes.count(b.street))
                continue;

            // Find the bounding box of the building
            float minLat = 90;
            float maxLat = 0;
            float minLon = 180;
            float maxLon = 0;
            
            for (const auto nodeId : b.nodes) {
                const auto node = refedBuildingNodes[osmIdToRefedBuildingNodeIdx[nodeId]];
                
                if (node.latitude < minLat)
                    minLat = node.latitude;

                if (node.longitude < minLon)
                    minLon = node.longitude;
                
                if (node.latitude > maxLat)
                    maxLat = node.latitude;
                
                if (node.longitude > maxLon)
                    maxLon = node.longitude;
            }

            const float searchLat = minLat + (maxLat - minLat) / 2;
            const float searchLon = minLon + (maxLon - minLon) / 2;

            // Find the closes node
            uint32_t minDist = infty;
            int64_t minDistNodeId = invalid_id;
            for (const auto nodeId : streetNameToStreetNodes[b.street]) {
                const auto node = refedNodes[osmIdToRefedNodeIdx[nodeId]];
                const auto dist = distanceBetweenNodes(node.latitude, node.longitude, searchLat, searchLon);

                if (dist < minDist) {
                    minDist = dist;
                    minDistNodeId = nodeId;
                }
            }

            if (b.housenumber == "" || (b.postcode == "" && b.city == ""))
                continue;
            
            std::string address = b.street + ";-;" + b.housenumber + ";-;" + b.postcode + ";-;" + b.city + ";-;" + std::to_string(minDistNodeId);
            addressList.push_back(address);
        }
    }

    OSMGraph buildGraph() {
        std::vector<uint32_t> firstOut;
        std::vector<uint32_t> heads;

        // Vertex attributes
        std::vector<float> lats;
        std::vector<float> longs;
        std::vector<int64_t> osmNodeIds;

        for (uint32_t v = 0; v < num_vertices; v++) {
            const auto node = refedNodes[v];
            assert(v == osmIdToRefedNodeIdx[node.id]);
            lats.push_back(node.latitude);
            longs.push_back(node.longitude);
            osmNodeIds.push_back(node.id);
        }

        std::stable_sort(graphEdges.begin(), graphEdges.end(), [](const GraphEdge &e1, const GraphEdge &e2) {
            return e1.to < e2.to;
        });

        std::stable_sort(graphEdges.begin(), graphEdges.end(), [](const GraphEdge &e1, const GraphEdge &e2) {
            return e1.from < e2.from;
        });
        
        // Edge attributes
        std::vector<uint32_t> travelTimes;
        std::vector<uint32_t> geoDistances;
        std::vector<int64_t> osmWayIds;

        uint32_t numEdges = 0;
        firstOut.push_back(0);

        uint32_t lastTail = 0;
        uint32_t lastHead = 0;
        for (const auto edge : graphEdges) {
            if (edge.from == lastTail && edge.to == lastHead) {
                std::cout << "Skip double edge!\n";
                continue;
            }
            
            while (lastTail < edge.from) {
                firstOut.push_back(numEdges);
                lastTail++;
            }

            heads.push_back(edge.to);
            travelTimes.push_back(edge.travelTime);
            geoDistances.push_back(edge.geoLength);
            osmWayIds.push_back(edge.osmWayId);
            numEdges++;
        }

        while (lastTail < num_vertices) {
            firstOut.push_back(numEdges);
            lastTail++;
        }

        OSMGraph g;
        g.bounds = bounds;
        g.firstOut = firstOut;
        g.heads = heads;
        g.geoDistances = geoDistances;
        g.travelTimes = travelTimes;
        g.lats = lats;
        g.longs = longs;
        g.osmNodeIds = osmNodeIds;
        g.osmWayIds = osmWayIds;
        g.addressList = addressList;

        return g;
    }

    Bounds bounds;
    std::unordered_set<int64_t> nodeIds;
    std::vector<Node> nodes;
    std::vector<Way> ways;
    std::vector<Building> buildings;

    std::vector<Edge> edges;
    std::vector<GraphEdge> graphEdges;

    uint32_t num_vertices;

    std::unordered_set<int64_t> refedNodeIds;
    std::vector<Node> refedNodes;
    std::unordered_map<int64_t, uint32_t> osmIdToRefedNodeIdx;

    std::unordered_set<int64_t> refedBuildingNodeIds;
    std::vector<Node> refedBuildingNodes;
    std::unordered_map<int64_t, uint32_t> osmIdToRefedBuildingNodeIdx;
    
    std::unordered_map<std::string, std::vector<int64_t>> streetNameToStreetNodes;
    std::unordered_map<std::string, int64_t> addressToOsmId;

    std::vector<std::string> addressList;
};
