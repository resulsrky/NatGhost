// burst_wawe.c - Symmetric NAT UDP Wave Burst

/*Motivasyon:Sysmmetric NAT arkasında olan cihazlar birbirlerine 
aynı anda tüm portları kaplayan bir UDP Wawe gönderirler
bu UDP Wawe ler çıkarken NAT Table da 1-5s lik registry oluşturur
bu süre içinde karşıdan gelen Wawe ile çarpışırsa veya 1-5sn içinde
Wawe NAT a ulaşırsa karşılıklı delik açılma olasılığı neredeyse %100
e çıkar çünkü Symmetric NAT ın kırılamamasının sebebi rastgele port atayarak
tahmin edilemezlik sağlamak ama bunun olasılık uzayı donanıma bağlı olarak
65.536 ile sınırlıdır bu sayı çok görünebilir ama bir bilgisayar için 
bu kadar porta aynı anda paket göndermek çokta zor değildir.
Şayet deliğin açılıp açılmadığını gönderilen 65.536 porttan dinlemek
tahmin edilebileceği üzere ciddi şekilde kaynak tüketir.Bu sebepten dolayı
portlar sadece gönderilmek için kullanılacak geri dönen cevaplar ise
promiscuous mode da dinlenecektir.*/

/*
  Bu sistem her biri 512 portluk wave gönderen 128 thread oluşturur.
  Her thread aynı anda çalışır. 
  Bu sayede symmetric NAT'ın tüm UDP port uzayı eş zamanlı doldurulur.
  Gerçek eş zamanlılık sağlanır. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#define THREAD_COUNT 128
#define PORTS_PER_THREAD 512
#define PAYLOAD_SIZE 20

const char *target_ip = "192.0.2.1"; // HEDEF IP BURAYA YAZ

void *burst_worker(void *arg) {
    int base_port = *(int*)arg;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        pthread_exit(NULL);
    }

    struct mmsghdr msgs[PORTS_PER_THREAD];
    struct iovec iovecs[PORTS_PER_THREAD];
    struct sockaddr_in addrs[PORTS_PER_THREAD];
    uint8_t payload[PORTS_PER_THREAD][PAYLOAD_SIZE];

    for (int i = 0; i < PORTS_PER_THREAD; i++) {
        snprintf((char *)payload[i], PAYLOAD_SIZE, "PORT%05d", base_port + i);
        iovecs[i].iov_base = payload[i];
        iovecs[i].iov_len  = PAYLOAD_SIZE;

        memset(&addrs[i], 0, sizeof(struct sockaddr_in));
        addrs[i].sin_family = AF_INET;
        addrs[i].sin_port = htons(base_port + i);
        inet_pton(AF_INET, target_ip, &addrs[i].sin_addr);

        memset(&msgs[i], 0, sizeof(struct mmsghdr));
        msgs[i].msg_hdr.msg_name = &addrs[i];
        msgs[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
        msgs[i].msg_hdr.msg_iov = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
    }

    int sent = sendmmsg(sock, msgs, PORTS_PER_THREAD, 0);
    printf("[Thread %d] Sent %d UDP packets (ports %d-%d)\n", base_port / PORTS_PER_THREAD, sent, base_port, base_port + PORTS_PER_THREAD - 1);

    close(sock);
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[THREAD_COUNT];
    int base_ports[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++) {
        base_ports[i] = i * PORTS_PER_THREAD;
        pthread_create(&threads[i], NULL, burst_worker, &base_ports[i]);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("[+] Burst wave complete.\n");
    return 0;
}
