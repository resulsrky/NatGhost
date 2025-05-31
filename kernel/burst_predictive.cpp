// burst_predictive.cpp - Predictive UDP burst tool for NAT penetration using learned port patterns

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>

#define PAYLOAD "PRED"  // 4 byte sabit tanımlama
#define PAYLOAD_SIZE 8  // sabit + port sayısı (2 byte) + 2 boşluk

void send_burst(const std::string& ip, const std::vector<uint16_t>& ports) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    char payload[PAYLOAD_SIZE];
    memset(payload, 0, PAYLOAD_SIZE);
    memcpy(payload, PAYLOAD, 4);

    for (auto port : ports) {
        addr.sin_port = htons(port);
        payload[4] = (port >> 8) & 0xFF;
        payload[5] = port & 0xFF;

        int sent = sendto(sock, payload, PAYLOAD_SIZE, 0,
                          (struct sockaddr*)&addr, sizeof(addr));
        if (sent < 0) perror("sendto");
    }

    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <target_ip> <port1> [port2] ...\n";
        return 1;
    }

    std::string target_ip = argv[1];
    std::vector<uint16_t> ports;

    for (int i = 2; i < argc; ++i) {
        ports.push_back(static_cast<uint16_t>(std::stoi(argv[i])));
    }

    std::cout << "[*] Sending burst to " << target_ip << " on " << ports.size() << " ports\n";
    send_burst(target_ip, ports);
    std::cout << "[+] Burst complete.\n";
    return 0;
}
