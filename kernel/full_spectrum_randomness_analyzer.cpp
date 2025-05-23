// randomness_analyzer.cpp - Analyze NAT port allocation randomness and extract patterns

#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <map>
#include <unordered_map>
#include <set>
#include <complex>


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

void modular_bias_analysis(const std::vector<PacketInfo>& data, int mod_base = 256) {
    std::vector<int> mod_counts(mod_base, 0);
    for (const auto& p : data) mod_counts[p.port % mod_base]++;

    int max_bucket = *std::max_element(mod_counts.begin(), mod_counts.end());
    int min_bucket = *std::min_element(mod_counts.begin(), mod_counts.end());

    std::cout << "\n[+] Modulo Bias Analysis (mod " << mod_base << "):\n";
    std::cout << "    Max Bucket: " << max_bucket << ", Min Bucket: " << min_bucket;
    if ((max_bucket - min_bucket) > (data.size() / (mod_base * 10))) {
        std::cout << " [! BIAS DETECTED]";
    }
    std::cout << "\n";
}

void windowed_entropy(const std::vector<PacketInfo>& data, size_t window_size = 128) {
    std::cout << "\n[+] Windowed Entropy Analysis (window = " << window_size << "):\n";
    for (size_t i = 0; i + window_size <= data.size(); i += window_size) {
        std::map<uint16_t, int> freq;
        for (size_t j = i; j < i + window_size; ++j) freq[data[j].port]++;
        double entropy = 0;
        for (const auto& [port, count] : freq) {
            double p = (double)count / window_size;
            entropy -= p * log2(p);
        }
        std::cout << "  Window [" << i << "–" << (i + window_size - 1) << "]: Entropy = " << entropy << "\n";
    }
}

void repetition_distance_analysis(const std::vector<PacketInfo>& data) {
    std::unordered_map<uint16_t, size_t> last_seen;
    std::map<size_t, int> repeat_distances;

    for (size_t i = 0; i < data.size(); ++i) {
        uint16_t port = data[i].port;
        if (last_seen.count(port)) {
            size_t dist = i - last_seen[port];
            repeat_distances[dist]++;
        }
        last_seen[port] = i;
    }

    std::cout << "\n[+] Repetition Distance Histogram (first 10):\n";
    int shown = 0;
    for (const auto& [dist, count] : repeat_distances) {
        std::cout << "  Distance " << dist << ": " << count << " times\n";
        if (++shown == 10) break;
    }
}

void predictive_hint_generator(const std::vector<PacketInfo>& data, size_t top_n = 10) {
    std::map<uint16_t, int> freq;
    for (const auto& p : data) freq[p.port]++;

    std::vector<std::pair<uint16_t, int>> sorted;
    for (const auto& [port, count] : freq) sorted.emplace_back(port, count);
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return b.second > a.second;
    });

    std::cout << "\n[+] Predictive Port Suggestions (top " << top_n << "):\n";
    for (size_t i = 0; i < std::min(top_n, sorted.size()); ++i) {
        std::cout << "  PORT " << sorted[i].first << " (" << sorted[i].second << " hits)\n";
    }
}

void autocorrelation_profile(const std::vector<PacketInfo>& data, size_t max_lag = 32) {
    std::cout << "\n[+] Autocorrelation Profile (lag 1–" << max_lag << "):\n";

    std::vector<double> ports;
    for (const auto& p : data) ports.push_back(static_cast<double>(p.port));

    double mean = std::accumulate(ports.begin(), ports.end(), 0.0) / ports.size();

    for (size_t lag = 1; lag <= max_lag; ++lag) {
        double numerator = 0, denom = 0;

        for (size_t i = 0; i + lag < ports.size(); ++i) {
            numerator += (ports[i] - mean) * (ports[i + lag] - mean);
        }

        for (size_t i = 0; i < ports.size(); ++i) {
            denom += (ports[i] - mean) * (ports[i] - mean);
        }

        double r = (denom != 0) ? numerator / denom : 0;
        printf("  Lag %2zu: r = %.5f\n", lag, r);
    }
}

void cycle_detector(const std::vector<PacketInfo>& data) {
    std::unordered_map<uint16_t, size_t> first_seen;
    std::map<size_t, int> cycle_lengths;

    for (size_t i = 0; i < data.size(); ++i) {
        uint16_t port = data[i].port;
        if (first_seen.count(port)) {
            size_t prev = first_seen[port];
            size_t cycle_len = i - prev;
            if (cycle_len > 1 && cycle_len < 1024) {  // filtreleme için mantıklı aralık
                cycle_lengths[cycle_len]++;
            }
        }
        first_seen[port] = i;
    }

    std::cout << "\n[+] Cycle Detection (first 10 frequent lengths):\n";
    int count = 0;
    for (const auto& [len, freq] : cycle_lengths) {
        std::cout << "  Cycle Length " << len << ": " << freq << " times\n";
        if (++count >= 10) break;
    }
}

void markov_predictor(const std::vector<PacketInfo>& data, size_t top_n = 5) {
    std::map<uint16_t, std::map<uint16_t, int>> transitions;

    for (size_t i = 1; i < data.size(); ++i) {
        uint16_t prev = data[i - 1].port;
        uint16_t curr = data[i].port;
        transitions[prev][curr]++;
    }

    std::cout << "\n[+] Markov Transition Probabilities (first " << top_n << " entries):\n";
    int count = 0;
    for (const auto& [from, to_map] : transitions) {
        std::cout << "  From PORT " << from << " → ";
        std::vector<std::pair<uint16_t, int>> sorted(to_map.begin(), to_map.end());
        std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return b.second < a.second; });

        for (size_t i = 0; i < std::min(top_n, sorted.size()); ++i) {
            std::cout << sorted[i].first << "(" << sorted[i].second << ") ";
        }
        std::cout << "\n";
        if (++count == top_n) break;
    }
}

void fourier_spectral_analysis(const std::vector<PacketInfo>& data, size_t max_freq = 64) {
    std::cout << "\n[+] Fourier Spectral Analysis (first " << max_freq << " frequency bins):\n";

    size_t N = data.size();
    if (N == 0) return;

    std::vector<std::complex<double>> spectrum(max_freq, 0);

    for (size_t k = 0; k < max_freq; ++k) {
        std::complex<double> sum = 0;
        for (size_t n = 0; n < N; ++n) {
            double angle = -2.0 * M_PI * k * n / N;
            sum += std::polar((double)data[n].port, angle);
        }
        spectrum[k] = sum;
    }

    for (size_t k = 0; k < max_freq; ++k) {
        double magnitude = std::abs(spectrum[k]) / N;
        printf("  Freq %2zu: Magnitude = %.5f\n", k, magnitude);
    }
}

void nist_rng_scorecard(const std::vector<PacketInfo>& data) {
    std::cout << "\n[+] NIST-Inspired Randomness Scorecard:\n";

    std::string bitstream;
    for (const auto& p : data) {
        for (int i = 15; i >= 0; --i) {
            bitstream += ((p.port >> i) & 1) ? '1' : '0';
        }
    }

    size_t ones = std::count(bitstream.begin(), bitstream.end(), '1');
    size_t zeros = bitstream.size() - ones;
    double freq_ratio = (double)ones / bitstream.size();

    std::cout << "  Frequency Test: 1s = " << ones << ", 0s = " << zeros << ", Ratio = " << freq_ratio << "\n";

    // Runs Test
    int runs = 1;
    for (size_t i = 1; i < bitstream.size(); ++i) {
        if (bitstream[i] != bitstream[i - 1]) runs++;
    }
    std::cout << "  Runs Test: " << runs << " total runs\n";

    // Longest Run of 1s
    size_t max_run = 0, curr_run = 0;
    for (char c : bitstream) {
        if (c == '1') {
            curr_run++;
            max_run = std::max(max_run, curr_run);
        } else {
            curr_run = 0;
        }
    }
    std::cout << "  Longest Run of 1s: " << max_run << "\n";

    // Approximate Entropy: kısa blok frekansları (örnekleme)
    std::map<std::string, int> freq2;
    for (size_t i = 0; i + 2 <= bitstream.size(); ++i) {
        freq2[bitstream.substr(i, 2)]++;
    }
    double H = 0;
    for (auto& [block, count] : freq2) {
        double p = (double)count / (bitstream.size() - 1);
        H -= p * log2(p);
    }
    std::cout << "  Approximate Entropy (2-bit blocks): " << H << " bits\n";
}

void bit_plane_entropy(const std::vector<PacketInfo>& data) {
    std::cout << "\n[+] Bit-Plane Entropy Analysis (bit 15 → 0):\n";

    const int bit_width = 16;
    std::vector<int> ones(bit_width, 0);
    std::vector<int> total(bit_width, data.size());

    for (const auto& p : data) {
        for (int i = 0; i < bit_width; ++i) {
            if ((p.port >> i) & 1) {
                ones[i]++;
            }
        }
    }

    for (int i = bit_width - 1; i >= 0; --i) {
        double p1 = (double)ones[i] / total[i];
        double p0 = 1.0 - p1;
        double H = 0.0;
        if (p1 > 0) H -= p1 * log2(p1);
        if (p0 > 0) H -= p0 * log2(p0);

        printf("  Bit %2d: Entropy = %.5f (1s: %d, 0s: %d)\n",
               i, H, ones[i], total[i] - ones[i]);
    }
}

void chi_square_uniformity_test(const std::vector<PacketInfo>& data, int bucket_count = 256) {
    std::cout << "\n[+] Chi-Square Uniformity Test (" << bucket_count << " buckets):\n";

    std::vector<int> buckets(bucket_count, 0);
    int total = data.size();

    for (const auto& p : data) {
        int bucket = p.port * bucket_count / 65536;
        buckets[bucket]++;
    }

    double expected = (double)total / bucket_count;
    double chi2 = 0;

    for (int i = 0; i < bucket_count; ++i) {
        double diff = buckets[i] - expected;
        chi2 += (diff * diff) / expected;
    }

    std::cout << "  Chi² = " << chi2 << ", expected range ≈ " << bucket_count << " ± sqrt(" << 2 * bucket_count << ")\n";

    if (chi2 > bucket_count + 2 * std::sqrt(2 * bucket_count)) {
        std::cout << "  [!] Uniformity Rejected: Distribution is likely biased\n";
    } else {
        std::cout << "  [+] Uniformity Accepted\n";
    }
}
int berlekamp_massey(const std::vector<int>& s) {
    int n = s.size();
    std::vector<int> c(n), b(n);
    c[0] = b[0] = 1;
    int l = 0, m = -1;

    for (int i = 0; i < n; ++i) {
        int d = s[i];
        for (int j = 1; j <= l; ++j)
            d ^= c[j] * s[i - j];

        if (d == 0) continue;

        std::vector<int> t = c;
        for (int j = 0; j < n - i + m; ++j)
            if (b[j]) c[j + i - m] ^= 1;

        if (2 * l <= i) {
            l = i + 1 - l;
            m = i;
            b = t;
        }
    }

    return l;
}

void linear_complexity(const std::vector<PacketInfo>& data) {
    std::cout << "\n[+] Linear Complexity (Berlekamp-Massey over LSB stream):\n";

    std::vector<int> bitstream;
    for (const auto& p : data) {
        for (int i = 15; i >= 0; --i) {
            bitstream.push_back((p.port >> i) & 1);
        }
    }

    int lfsr_len = berlekamp_massey(bitstream);
    std::cout << "  Estimated LFSR length: " << lfsr_len << " (max " << bitstream.size() << ")\n";
}
std::vector<int> lfsr_predictor(const std::vector<int>& bitstream, int l, int predict_count) {
    std::vector<int> c(l + 1, 0);
    std::vector<int> b(l + 1, 0);
    std::vector<int> s = bitstream;

    // Berlekamp-Massey: bu kez katsayıları da elde ediyoruz
    c[0] = b[0] = 1;
    int m = -1;

    for (int i = 0; i < (int)s.size(); ++i) {
        int d = s[i];
        for (int j = 1; j <= l; ++j)
            d ^= c[j] * s[i - j];

        if (d == 0) continue;

        std::vector<int> t = c;
        for (int j = 0; j < l; ++j)
            if (b[j]) c[j + i - m] ^= 1;

        if (2 * l <= i) {
            l = i + 1 - l;
            m = i;
            b = t;
        }
    }

    // Tahmin et: LFSR simülasyonu
    std::vector<int> predicted;
    std::vector<int> state = std::vector<int>(s.end() - l, s.end());

    for (int i = 0; i < predict_count; ++i) {
        int next = 0;
        for (int j = 1; j <= l; ++j)
            next ^= c[j] * state[l - j];
        predicted.push_back(next);

        state.erase(state.begin());
        state.push_back(next);
    }

    return predicted;
}
void predict_next_bits(const std::vector<PacketInfo>& data) {
    std::vector<int> bitstream;
    for (const auto& p : data) {
        for (int i = 15; i >= 0; --i) {
            bitstream.push_back((p.port >> i) & 1);
        }
    }

    int l = berlekamp_massey(bitstream);
    auto next_bits = lfsr_predictor(bitstream, l, 64);

    std::cout << "\n[+] LFSR-based Bit Prediction (first 64 bits):\n";
    for (size_t i = 0; i < next_bits.size(); ++i) {
        std::cout << next_bits[i];
        if ((i + 1) % 8 == 0) std::cout << " ";
    }
    std::cout << "\n";
}
void predict_next_ports(const std::vector<PacketInfo>& data) {
    std::cout << "\n[+] LFSR-based Port Prediction (first 10):\n";

    std::vector<int> bitstream;
    for (const auto& p : data) {
        for (int i = 15; i >= 0; --i) {
            bitstream.push_back((p.port >> i) & 1);
        }
    }

    int l = berlekamp_massey(bitstream);
    auto next_bits = lfsr_predictor(bitstream, l, 10);

    for (size_t i = 0; i < next_bits.size(); ++i) {
        std::cout << "Predicted PORT: " << (next_bits[i] % 65536) << "\n";
    }
}

void random_walk_analysis(const std::vector<PacketInfo>& data) {
    std::cout << "\n[+] Random Walk Analysis:\n";

    std::vector<int> walk(data.size());
    for (size_t i = 1; i < data.size(); ++i) {
        walk[i] = walk[i - 1] + (data[i].port - data[i - 1].port);
    }

    // Placeholder for actual random walk analysis
    std::cout << "  [!] Random walk analysis not implemented\n";
}
void autocorrelation_analysis(const std::vector<PacketInfo>& data, size_t max_lag = 32) {
    std::cout << "\n[+] Autocorrelation Analysis (lag 1–" << max_lag << "):\n";

    std::vector<double> ports;
    for (const auto& p : data) ports.push_back(static_cast<double>(p.port));

    double mean = std::accumulate(ports.begin(), ports.end(), 0.0) / ports.size();

    for (size_t lag = 1; lag <= max_lag; ++lag) {
        double numerator = 0, denom = 0;

        for (size_t i = 0; i + lag < ports.size(); ++i) {
            numerator += (ports[i] - mean) * (ports[i + lag] - mean);
        }

        for (size_t i = 0; i < ports.size(); ++i) {
            denom += (ports[i] - mean) * (ports[i] - mean);
        }

        double r = (denom != 0) ? numerator / denom : 0;
        printf("  Lag %2zu: r = %.5f\n", lag, r);
    }
}

void chi_square_uniformity_test(const std::vector<PacketInfo>& data, int bucket_count = 256) {
    std::cout << "\n[+] Chi-Square Uniformity Test (" << bucket_count << " buckets):\n";

    std::vector<int> buckets(bucket_count, 0);
    int total = data.size();

    for (const auto& p : data) {
        int bucket = p.port * bucket_count / 65536;
        buckets[bucket]++;
    }

    double expected = (double)total / bucket_count;
    double chi2 = 0;

    for (int i = 0; i < bucket_count; ++i) {
        double diff = buckets[i] - expected;
        chi2 += (diff * diff) / expected;
    }

    std::cout << "  Chi² = " << chi2 << ", expected range ≈ " << bucket_count << " ± sqrt(" << 2 * bucket_count << ")\n";

    if (chi2 > bucket_count + 2 * std::sqrt(2 * bucket_count)) {
        std::cout << "  [!] Uniformity Rejected: Distribution is likely biased\n";
    } else {
        std::cout << "  [+] Uniformity Accepted\n";
    }
}

void kolmogorov_smirnov_test(const std::vector<PacketInfo>& data) {
    std::cout << "\n[+] Kolmogorov-Smirnov Uniformity Test:\n";

    std::vector<uint16_t> sorted_ports;
    for (const auto& p : data) sorted_ports.push_back(p.port);
    std::sort(sorted_ports.begin(), sorted_ports.end());

    size_t n = sorted_ports.size();
    double D_max = 0.0;

    for (size_t i = 0; i < n; ++i) {
        double F_obs = (double)(i + 1) / n;
        double F_exp = (double)sorted_ports[i] / 65536.0;
        double diff = std::abs(F_obs - F_exp);
        D_max = std::max(D_max, diff);
    }

    double threshold = 1.36 / std::sqrt(n);  // Approx for alpha = 0.05
    std::cout << "  D_max = " << D_max << ", Threshold ≈ " << threshold << "\n";

    if (D_max > threshold) {
        std::cout << "  [!] K-S Test Failed: Distribution deviates from uniform\n";
    } else {
        std::cout << "  [+] K-S Test Passed: Uniformity plausible\n";
    }
}

int berlekamp_massey(const std::vector<int>& s) {
    int n = s.size();
    std::vector<int> c(n), b(n);
    c[0] = b[0] = 1;
    int l = 0, m = -1;

    for (int i = 0; i < n; ++i) {
        int d = s[i];
        for (int j = 1; j <= l; ++j)
            d ^= c[j] * s[i - j];
        if (d == 0) continue;

        std::vector<int> t = c;
        for (int j = 0; j < n - i + m; ++j)
            if (b[j]) c[j + i - m] ^= 1;

        if (2 * l <= i) {
            l = i + 1 - l;
            m = i;
            b = t;
        }
    }
    return l;
}

// Tahminci LFSR
std::vector<int> lfsr_predictor(const std::vector<int>& bitstream, int l, int predict_count) {
    std::vector<int> c(l + 1, 0);
    std::vector<int> b(l + 1, 0);
    std::vector<int> s = bitstream;

    c[0] = b[0] = 1;
    int m = -1;

    for (int i = 0; i < (int)s.size(); ++i) {
        int d = s[i];
        for (int j = 1; j <= l; ++j)
            d ^= c[j] * s[i - j];

        if (d == 0) continue;
        std::vector<int> t = c;
        for (int j = 0; j < l; ++j)
            if (b[j]) c[j + i - m] ^= 1;

        if (2 * l <= i) {
            l = i + 1 - l;
            m = i;
            b = t;
        }
    }

    std::vector<int> predicted;
    std::vector<int> state = std::vector<int>(s.end() - l, s.end());
    for (int i = 0; i < predict_count; ++i) {
        int next = 0;
        for (int j = 1; j <= l; ++j)
            next ^= c[j] * state[l - j];
        predicted.push_back(next);
        state.erase(state.begin());
        state.push_back(next);
    }
    return predicted;
}

int berlekamp_massey(const std::vector<int>& s) {
    int n = s.size();
    std::vector<int> c(n), b(n);
    c[0] = b[0] = 1;
    int l = 0, m = -1;

    for (int i = 0; i < n; ++i) {
        int d = s[i];
        for (int j = 1; j <= l; ++j)
            d ^= c[j] * s[i - j];
        if (d == 0) continue;

        std::vector<int> t = c;
        for (int j = 0; j < n - i + m; ++j)
            if (b[j]) c[j + i - m] ^= 1;

        if (2 * l <= i) {
            l = i + 1 - l;
            m = i;
            b = t;
        }
    }
    return l;
}

// Tahminci LFSR
std::vector<int> lfsr_predictor(const std::vector<int>& bitstream, int l, int predict_count) {
    std::vector<int> c(l + 1, 0);
    std::vector<int> b(l + 1, 0);
    std::vector<int> s = bitstream;

    c[0] = b[0] = 1;
    int m = -1;

    for (int i = 0; i < (int)s.size(); ++i) {
        int d = s[i];
        for (int j = 1; j <= l; ++j)
            d ^= c[j] * s[i - j];

        if (d == 0) continue;
        std::vector<int> t = c;
        for (int j = 0; j < l; ++j)
            if (b[j]) c[j + i - m] ^= 1;

        if (2 * l <= i) {
            l = i + 1 - l;
            m = i;
            b = t;
        }
    }

    std::vector<int> predicted;
    std::vector<int> state = std::vector<int>(s.end() - l, s.end());
    for (int i = 0; i < predict_count; ++i) {
        int next = 0;
        for (int j = 1; j <= l; ++j)
            next ^= c[j] * state[l - j];
        predicted.push_back(next);
        state.erase(state.begin());
        state.push_back(next);
    }
    return predicted;
}
// Veri yükleyici
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


int main() {
    std::string filename = "packet_log.txt";
    std::vector<PacketInfo> packets = load_packet_data(filename);
    if (packets.empty()) return 1;

    std::cout << "[*] Toplam Paket: " << packets.size() << "\n";

    delta_port_analysis(packets);
    entropy_analysis(packets);
    time_clustering(packets);
    modular_bias_analysis(packets);
    windowed_entropy(packets);
    repetition_distance_analysis(packets);
    predictive_hint_generator(packets);

     std::vector<int> bitstream;
    for (const auto& p : packets) {
        for (int i = 15; i >= 0; --i) {
            bitstream.push_back((p.port >> i) & 1);
        }
    }

    // Berlekamp-Massey ile LFSR uzunluğunu bul
    int lfsr_len = berlekamp_massey(bitstream);
    std::cout << "\n[+] Berlekamp-Massey LFSR Length: " << lfsr_len << "\n";

    // Tahmin yap
    auto predicted_bits = lfsr_predictor(bitstream, lfsr_len, 64);
    std::cout << "[+] First 64 Predicted Bits:\n";
    for (size_t i = 0; i < predicted_bits.size(); ++i) {
        std::cout << predicted_bits[i];
        if ((i + 1) % 8 == 0) std::cout << " ";
    }
    std::cout << "\n";

    // Opsiyonel: Tahmin edilen bitleri 16-bit portlara dönüştür
    std::vector<uint16_t> predicted_ports;
    for (size_t i = 0; i + 15 < predicted_bits.size(); i += 16) {
        uint16_t port = 0;
        for (int j = 0; j < 16; ++j) {
            port = (port << 1) | predicted_bits[i + j];
        }
        predicted_ports.push_back(port);
    }

    std::cout << "[+] Predicted Ports (from bits):\n";
    for (auto port : predicted_ports) {
        std::cout << "  " << port << "\n";
    }


    return 0;
}