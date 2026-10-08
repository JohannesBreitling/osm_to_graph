
#include <string>


#include "../external/pugixml/pugixml.hpp"
#include "./osm_graph_builder.hpp"

#pragma once

class PugixmlAdapter {

public:
    PugixmlAdapter(const char* path, OSMGraphBuilder &builder_) : builder(builder_) {
        std::cout << "Parsing the xml....." << std::flush;
        pugi::xml_parse_result result = doc.load_file(path);
        
        if (!result) {
            throw std::runtime_error("XML parsed with errors, exiting now.");
        }

        std::cout << "done.\n";
    }

    void parseDocument() {
        std::cout << "Read in the osm data....." << std::flush;
        auto rootNode = doc.child("osm");
        std::string rootName = std::string(rootNode.name());

        if (rootName != "osm") {
            std::string err = "Unexpected format! First child should be <osm>, but is " + std::string(rootNode.name()) + "!";
            throw std::runtime_error(err);
        }

        for (const auto xml_node : rootNode.children()) {
            std::string nodeName(xml_node.name());

            if (nodeName == "bounds") {
                parseBounds(xml_node);
                continue;
            }

            if (nodeName == "node") {
                parseNode(xml_node);
                continue;
            }

            if (nodeName == "relation") {
                continue;
            }

            if (nodeName == "way") {
                parseWay(xml_node);
                continue;
            }
            
            std::cerr << "Unknow node type: " << xml_node.name() << "\n";
        }

        std::cout << "done.\n";
    }

private:

    void parseBounds(const pugi::xml_node &currentNode) {
        // TODO: Error handling
        const auto minLatAttr = currentNode.attribute("minlat").value();
        const auto maxLatAttr = currentNode.attribute("maxlat").value();
        const auto minLongAttr = currentNode.attribute("minlon").value();
        const auto maxLongAttr = currentNode.attribute("maxlon").value();
        Bounds bounds = {std::strtof(minLatAttr, nullptr), std::strtof(maxLatAttr, nullptr), std::strtof(minLongAttr, nullptr), std::strtof(maxLongAttr, nullptr)};
        builder.setBounds(bounds);
    }

    void parseNode(const pugi::xml_node &currentNode) {
        // TODO: Error handling
        const auto idAttr = currentNode.attribute("id").value();
        const auto latAttr = currentNode.attribute("lat").value();
        const auto longAttr = currentNode.attribute("lon").value();
        Node n = {static_cast<int64_t>(std::atoll(idAttr)), std::strtof(latAttr, nullptr), std::strtof(longAttr, nullptr)};
        builder.addNode(n);

        if (currentNode.children().empty())
            return;

        bool isBuilding = false;
        Building building;
        for (const auto child : currentNode.children()) {
            const auto key = std::string(child.attribute("k").value());
            const auto value = std::string(child.attribute("v").value());

            if (key == "addr:street") {
                isBuilding = true;
                building.street = value;
                continue;
            }

            if (key == "addr:housenumber") {
                isBuilding = true;
                building.housenumber = value;
                continue;
            }

            if (key == "addr:postcode") {
                isBuilding = true;
                building.postcode = value;
                continue;
            }

            if (key == "addr:city") {
                isBuilding = true;
                building.city = value;
                continue;
            }
        }

        if (!isBuilding)
            return;
        
        building.nodes = std::vector<int64_t>({n.id});
        builder.addBuilding(building);
    }

    void parseWay(const pugi::xml_node &currentNode) {
        // TODO: Error handling
        std::vector<int64_t> nodes;
        int64_t wayId = static_cast<int64_t>(std::atoll(currentNode.attribute("id").value()));
        bool isHighway = false;
        bool isBuilding = false;
        Building building;
        bool oneway = false;
        std::string name = "";
        uint32_t maxSpeed = 0;
        std::string type;
        for (const auto &child : currentNode.children()) {
            if (std::string(child.name()) == "nd") {
                // Add the node reference
                const auto ndRefStr = child.attribute("ref").value();
                const auto ndRef = static_cast<int64_t>(std::atoll(ndRefStr));
                nodes.push_back(ndRef);
                continue;
            }

            if (std::string(child.name()) == "tag") {
                // Add the node reference
                const auto key = std::string(child.attribute("k").value());
                const auto value = std::string(child.attribute("v").value());
                if (key == "highway") {
                    isHighway = true;
                    if (!(value == "motorway"
                       || value == "trunk"
                       || value == "primary"
                       || value == "secondary"
                       || value == "tertiary"
                       || value == "unclassified"
                       || value == "residential"
                       || value == "motorway_link"
                       || value == "trunk_link"
                       || value == "primary_link"
                       || value == "secondary_link"
                       || value == "tertiary_link"
                       || value == "living_street"
                       || value == "service"
                       || value == "track"
                       || value == "road")
                    ) {
                        return;
                    }
                    
                    type = value;

                    // Go to the next tag
                    continue;
                }

                if (key == "maxspeed") {
                    if (value == "walk" || value == "DE:walk" || value == "DE:living_street") {
                        maxSpeed = 7;
                    } else if (value == "none" || value == "DE:rural") {
                        maxSpeed = 100;
                    } else if (value == "DE:urban") {
                        maxSpeed = 50;
                    } else if (value == "signals") {
                        continue;
                    } else {
                        maxSpeed = static_cast<uint32_t>(std::stoul(value));
                    }  
                    continue;
                }

                if (key == "oneway") {
                    oneway = (value == "true" || value == "yes") ? true : false;
                    continue;
                }

                if (key == "name") {
                    name = value;
                    continue;
                }

                if (key == "addr:street") {
                    isBuilding = true;
                    building.street = value;
                    continue;
                }

                if (key == "addr:housenumber") {
                    isBuilding = true;
                    building.housenumber = value;
                    continue;
                }

                if (key == "addr:postcode") {
                    isBuilding = true;
                    building.postcode = value;
                    continue;
                }

                if (key == "addr:city") {
                    isBuilding = true;
                    building.city = value;
                    continue;
                }
            }
        }
        
        if (isHighway && maxSpeed == 0) {
            maxSpeed = 50;
        }

        if (isHighway) {
            Way way;
            way.id = wayId;
            way.oneway = oneway;
            way.maxSpeed = maxSpeed;
            way.nodes = nodes;
            way.streetName = name;
            way.type = type;
            builder.addWay(way);
        }

        if (isBuilding) {
            building.nodes = nodes;
            builder.addBuilding(building);
        }
    }
    
    pugi::xml_document doc;
    OSMGraphBuilder &builder;

};