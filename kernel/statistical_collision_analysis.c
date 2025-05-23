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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <pthread.h>
#include <fcntl.h>

#define MAX_PORT 65535
#define MAX_THREADS 100
#define PACKET_SIZE 4096
#define TIMEOUT 5

typedef struct {
    uint16_t port;
    int sockfd;
} thread_data_t;

struct sockaddr server;
thread_data_t sockets[MAX_THREADS];
thread_data_t *socket_list[MAX_THREADS];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void send_request(int idx){}