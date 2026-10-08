
#include <cstdint>
#include <fstream>
#include <vector>

#pragma once

template <typename T>
void write_vector_to_file(const std::vector<T> &vec, const std::string &path) {
    std::ofstream out(path, std::ios::binary);

    if (!out) {
        std::cerr << "Something went wrong creating / opening the output file.\n";
        exit(1);
    }

    uint32_t size = vec.size();
    if (!vec.empty()) {
        out.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
    }
    out.close();
}

template <typename T>
std::vector<T> read_vector_from_file(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "Something went wrong opening the file.\n";
        exit(1);
    };

    in.seekg(0, std::ios::end);
	uint64_t file_size = in.tellg();
    in.seekg(0, std::ios::beg);
    
    std::vector<T> vec(file_size / sizeof(T));
    
    if (file_size > 0 && file_size % sizeof(T) == 0) {
        in.read(reinterpret_cast<char*>(vec.data()), file_size);
    }
    
    in.close();
    return vec;
}

void write_string_list_to_file(const std::vector<std::string> &list, const std::string &path) {
    std::ofstream outFile(path);
    for (const auto line : list) {
        outFile << line << "\n";    
    }
    outFile.close();
}
