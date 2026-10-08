
#include <iostream>
#include <string>

#include "../external/pugixml/pugixml.hpp"

#include "../src/pugixml_adapter.hpp"
#include "../src/osm_graph_builder.hpp"
#include "../src/data/data.hpp"

int main(int argc, char** argv) {
    std::cout << "- - - OSM data to road graph conversion - - -\n";

    if (argc != 3) {
        std::cerr << "Unexpected number of arguments. Expected ./convert_osm_to_graph <path_to_osm_file> <path_to_output_dir>\n";
        return 1;
    }

    OSMGraphBuilder builder;

    PugixmlAdapter adapter(argv[1], builder);
    adapter.parseDocument();

    const auto g = builder.finalize();
    write_osm_graph_to_file(g, argv[2]);
}