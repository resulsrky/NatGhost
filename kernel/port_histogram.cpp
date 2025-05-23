// port_histogram.cpp - Analyze shared memory NAT port collisions and extract dense ranges

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

#define SHM_NAME "/nat_collision_map"
#define PORT_COUNT 65536
#define TOP_N 2048

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd < 0) {
        perror("shm_open");
        return 1;
    }

    uint32_t* collisions = (uint32_t*) mmap(NULL, sizeof(uint32_t) * PORT_COUNT, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (collisions == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    std::vector<std::pair<int, uint32_t>> histogram;
    histogram.reserve(PORT_COUNT);

    for (int i = 0; i < PORT_COUNT; ++i) {
        if (collisions[i] > 0) {
            histogram.emplace_back(i, collisions[i]);
        }
    }

    std::sort(histogram.begin(), histogram.end(), [](const auto& a, const auto& b) {
        return b.second < a.second; // descending order by count
    });

    std::cout << "Top " << TOP_N << " ports by collision count:\n";
    for (int i = 0; i < std::min((int)histogram.size(), TOP_N); ++i) {
        std::cout << "PORT " << histogram[i].first << " -> " << histogram[i].second << " hits\n";
    }

    // İstenirse bu yoğun portlar bir dosyaya yazılabilir
    std::ofstream out("hot_ports.txt");
    for (int i = 0; i < std::min((int)histogram.size(), TOP_N); ++i) {
        out << histogram[i].first << "\n";
    }
    out.close();

    munmap(collisions, sizeof(uint32_t) * PORT_COUNT);
    close(shm_fd);
    return 0;
}
