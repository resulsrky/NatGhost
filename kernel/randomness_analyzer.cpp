// randomness_analyzer.cpp - Analyze NAT port allocation randomness and extract patterns

#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <map>

struct PacketInfo {
    uint16_t port;
    uint64_t timestamp_us;
    uint32_t seq_index;
};

std::vector<PacketInfo> load_packet_data(const std::string &filename) {
    std::vector<PacketInfo> data;
    std::ifstream f(filename);
    if (!f.is_open()) {
        std::cerr << "[!] Dosya açılmadı: " << filename << std::endl;
        return data;
    }
    PacketInfo p;
    while (f >> p.port >> p.timestamp_us >> p.seq_index) {
        data.push_back(p);
    }
    return data;
}

void delta_port_analysis(const std::vector<PacketInfo>& data) {
    std::map<int, int> delta_hist;
    for (size_t i = 1; i < data.size(); ++i) {
        int delta = (int)data[i].port - (int)data[i - 1].port;
        delta_hist[delta]++;
    }
    std::cout << "\n[+] Δport Histogram (first 10 deltas):\n";
    int count = 0;
    for (const auto& [d, c] : delta_hist) {
        std::cout << "Δ " << d << ": " << c << "\n";
        if (++count == 10) break;
    }
}

void entropy_analysis(const std::vector<PacketInfo>& data) {
    std::map<uint16_t, int> freq;
    for (const auto& p : data) freq[p.port]++;
    double entropy = 0;
    int total = data.size();
    for (const auto& [port, count] : freq) {
        double p = (double)count / total;
        entropy -= p * log2(p);
    }
    std::cout << "\n[+] Shannon Entropy: " << entropy << " bits (" << total << " paket)\n";
}

void time_clustering(const std::vector<PacketInfo>& data) {
    std::cout << "\n[+] Zaman Aralıkları (μs cinsinden):\n";
    for (size_t i = 1; i < std::min(data.size(), size_t(10)); ++i) {
        std::cout << "Δt = " << (data[i].timestamp_us - data[i-1].timestamp_us) << " μs\n";
    }
}

int main() {
    std::string filename = "packet_log.txt";
    std::vector<PacketInfo> packets = load_packet_data(filename);
    if (packets.empty()) return 1;

    std::cout << "[*] Toplam Paket: " << packets.size() << "\n";

    delta_port_analysis(packets);
    entropy_analysis(packets);
    time_clustering(packets);

    return 0;
}
