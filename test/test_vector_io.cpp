
#include <iostream>
#include "../src/vector_util.hpp"

int main() {


    std::vector<uint32_t> data1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    write_vector_to_file<uint32_t>(data1, "./output/temp/vec");
    const auto vec1 = read_vector_from_file<uint32_t>("./output/temp/vec");
    for (const auto elem : vec1) {
        std::cout << elem << " ";
    }
    std::cout << "\n";



    std::vector<int64_t> data2 = {-1, 2, -3, 400000, 205, 6, 7, 8, 0, -10};
    write_vector_to_file<int64_t>(data2, "./output/temp/vec2");
    const auto vec2 = read_vector_from_file<int64_t>("./output/temp/vec2");
    for (const auto elem : vec2) {
        std::cout << elem << " ";
    }
    std::cout << "\n";

    std::vector<float> data3 = {1.1, 2.3, 3.4, 4.5, 5.0, 6.f, 71200.22, 8.33333, 9.1, 10.f};
    write_vector_to_file<float>(data3, "./output/temp/vec");
    const auto vec3 = read_vector_from_file<float>("./output/temp/vec");
    for (const auto elem : vec3) {
        std::cout << elem << " ";
    }
    std::cout << "\n";

    return 0;
}