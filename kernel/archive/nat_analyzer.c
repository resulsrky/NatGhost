#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAGIC_COOKIE 0x2112A442

typedef struct {
    char ip[INET_ADDRSTRLEN];
    uint16_t port;
} mapped_addr_t;

int get_mapped_address(const char *stun_host, uint16_t stun_port, uint16_t client_port, mapped_addr_t *result) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in local = {0};
    local.sin_family = AF_INET;
    local.sin_port = htons(client_port);
    local.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0) {
        close(sock);
        return -1;
    }

    struct sockaddr_in server = {0};
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    char portstr[6]; snprintf(portstr, sizeof(portstr), "%u", stun_port);
    if (getaddrinfo(stun_host, portstr, &hints, &res) != 0) {
        close(sock); return -1;
    }
    server = *(struct sockaddr_in*)res->ai_addr;
    freeaddrinfo(res);

    uint8_t req[20];
    memset(req, 0, sizeof(req));
    req[0] = 0x00; req[1] = 0x01; // Binding Request
    req[2] = 0x00; req[3] = 0x00; // Length
    uint32_t cookie = htonl(MAGIC_COOKIE);
    memcpy(&req[4], &cookie, 4);
    for (int i = 8; i < 20; i++) req[i] = rand() & 0xFF;

    sendto(sock, req, 20, 0, (struct sockaddr*)&server, sizeof(server));

    struct timeval tv = {2, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    uint8_t buf[1024];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fromlen);
    close(sock);
    if (len <= 0) return -1;

    for (int i = 20; i < len - 8; i++) {
        if (buf[i] == 0x00 && buf[i+1] == 0x20) { // XOR-MAPPED-ADDRESS
            uint16_t xor_port;
            memcpy(&xor_port, buf + i + 6, 2);
            xor_port ^= (MAGIC_COOKIE >> 16);
            result->port = ntohs(xor_port);

            uint32_t xor_ip;
            memcpy(&xor_ip, buf + i + 8, 4);
            xor_ip ^= htonl(MAGIC_COOKIE);
            inet_ntop(AF_INET, &xor_ip, result->ip, INET_ADDRSTRLEN);
            return 0;
        }
    }
    return -1;
}

void analyze_nat_type() {
    const char *servers[] = {
        "stun.l.google.com",
        "stun1.l.google.com",
        "stun2.l.google.com"
    };

    mapped_addr_t results[3];
    int ok = 0;
    uint16_t local_port = 54321; // rastgele bir yüksek port

    printf("NAT Tipi Analizi Başlatılıyor...\n\n");

    for (int i = 0; i < 3; i++) {
        if (get_mapped_address(servers[i], 19302, local_port, &results[i]) == 0) {
            printf("[%d] %s:%d\n", i + 1, results[i].ip, results[i].port);
            ok++;
        } else {
            printf("[%d] %s: başarısız\n", i + 1, servers[i]);
        }
        sleep(1); // kısa bekleme
    }

    if (ok < 2) {
        printf("\n⚠️ Yeterli STUN cevabı alınamadı. NAT tipi tespit edilemedi.\n");
        return;
    }

    int same_ip = strcmp(results[0].ip, results[1].ip) == 0;
    int same_port = results[0].port == results[1].port;

    printf("\n📊 NAT Analizi Sonucu:\n");
    if (same_ip && same_port) {
        printf("✅ NAT Tipi: Full Cone / Port-Preserving NAT (açık)\n");
    } else if (same_ip && !same_port) {
        printf("🟡 NAT Tipi: Restricted or Port-Restricted NAT\n");
    } else {
        printf("🔴 NAT Tipi: Symmetric NAT (her sunucuya farklı port/IP)\n");
    }
}
int main() {
    srand(time(NULL));
    analyze_nat_type();
    return 0;
}
