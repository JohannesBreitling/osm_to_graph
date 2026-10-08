
#include <iostream>
#include "../src/vector_util.hpp"

int main() {

    const auto osmIds = read_vector_from_file<int64_t>("./output/karlsruhe/osm_node_ids");
    const auto travelTimes = read_vector_from_file<uint32_t>("./output/karlsruhe/travel_times");

    for (size_t i = 0; i < 20; i++) {
        std::cout <<  osmIds[i] << " - " << travelTimes[i] << "\n";
    }

    return 0;
}