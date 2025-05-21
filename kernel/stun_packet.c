#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>        // getaddrinfo

#define MESSAGE_TYPE   0x0001
#define MESSAGE_LENGTH 0
#define MAGIC_COOKIE   0x2112A442
#define RESPONSE_BUF   1024

typedef struct {
    uint8_t transaction_id[12];
} stun_transaction_t;

typedef struct {
    uint16_t m_type;
    uint16_t m_length;
    uint32_t magic_cookie;
    stun_transaction_t transaction_id;
} stun_header_t;

// STUN header'ını oluşturur
stun_header_t *generate_stun_packet() {
    stun_header_t *pkt = malloc(sizeof(stun_header_t));
    if (!pkt) { perror("malloc"); exit(EXIT_FAILURE); }
    pkt->m_type       = htons(MESSAGE_TYPE);
    pkt->m_length     = htons(MESSAGE_LENGTH);
    pkt->magic_cookie = htonl(MAGIC_COOKIE);
    for (int i = 0; i < 12; i++)
        pkt->transaction_id.transaction_id[i] = rand() & 0xFF;

    printf("-> STUN Binding Request:\n");
    printf("   Type: 0x%04x, Length: %u, Cookie: 0x%08x\n",
           ntohs(pkt->m_type),
           ntohs(pkt->m_length),
           ntohl(pkt->magic_cookie));
    printf("   Transaction ID: ");
    for (int i = 0; i < 12; i++) printf("%02x ", pkt->transaction_id.transaction_id[i]);
    printf("\n\n");
    return pkt;
}

int stun_message(const char *server_host, uint16_t server_port,
                 uint16_t client_port)
{
    // 1) UDP soket oluştur
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return -1; }

    // 2) Dinleme (bind) — tüm arayüzlerde client_port'u aç
    struct sockaddr_in local = {0};
    local.sin_family      = AF_INET;
    local.sin_port        = htons(client_port);
    local.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0) {
        perror("bind"); close(sock); return -1;
    }

    // 3) STUN paketi oluştur
    stun_header_t *req = generate_stun_packet();

    // 4) Hostname -> IP (getaddrinfo)
    struct addrinfo hints = {0}, *res;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    char portstr[6];
    snprintf(portstr, sizeof(portstr), "%u", server_port);
    if (getaddrinfo(server_host, portstr, &hints, &res) != 0) {
        fprintf(stderr, "DNS lookup failed for %s\n", server_host);
        free(req);
        close(sock);
        return -1;
    }
    struct sockaddr_in server = *(struct sockaddr_in*)res->ai_addr;
    freeaddrinfo(res);

    // 5) İsteği gönder
    ssize_t sent = sendto(sock, req, sizeof(*req), 0,
                          (struct sockaddr*)&server, sizeof(server));
    if (sent != sizeof(*req)) {
        perror("sendto"); free(req); close(sock); return -1;
    }
    printf("[✓] STUN Request sent (%zd bytes)\n\n", sent);
    free(req);

    // 6) Cevabı al (3 sn timeout)
    struct timeval tv = {3, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    uint8_t buf[RESPONSE_BUF];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    ssize_t len = recvfrom(sock, buf, sizeof(buf), 0,
                           (struct sockaddr*)&from, &fromlen);
    if (len < 0) {
        perror("recvfrom");
        printf("(!) No STUN response within timeout.\n");
        close(sock);
        return -1;
    }

    // 7) Gelen cevabı yazdır
    char from_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from.sin_addr, from_ip, sizeof(from_ip));
    printf("[✓] Response from %s:%u (%zd bytes)\n",
           from_ip, ntohs(from.sin_port), len);
    printf("    Data: ");
    for (ssize_t i = 0; i < len; i++) printf("%02x ", buf[i]);
    printf("\n\n");

    close(sock);
    return 0;
}

int main() {
    srand((unsigned)time(NULL));  // transaction ID için

    const char *stun_srv   = "stun.l.google.com";
    uint16_t      stun_port = 19302;
    uint16_t      client_port = 55555;  // >1023

    printf("STUN sunucusu: %s:%u, dinleme portu: %u\n\n",
           stun_srv, stun_port, client_port);

    if (stun_message(stun_srv, stun_port, client_port) == 0)
        printf("STUN işlemi başarıyla tamamlandı.\n");
    else
        printf("STUN işlemi başarısız oldu.\n");

    return 0;
}
