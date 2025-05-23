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

#define THREAD_COUNT 128
#define PORTS_PER_THREAD 512
#define PAYLOAD_SIZE 20

const char *target_ip = "";

void *burster(void *arg){
    int base_port = *(int *)args;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0){
		perror("Socket failure");
		pthread_exit(NULL);
	}

	struct mmsghed msgs[PORTS_PER_THREAD];
	struct iovec iov[PORTS_PER_THREAD];
	struct sockaddr_in addrs[PORTS_PER_THREAD];
	uint8_t payload[PORTS_PER_THREAD][PAYLOAD_SIZE];

		for(int i = 0; i < PORTS_PER_THREAD ;i++){
	    	snprintf((char *)payload[i], PAYLOAD_SIZE, "PORT%05d", base_port + i);
			iov[i].iov_base = payload[i];
			iov[i].iov_len = PAYLOAD_SIZE;

			memset(&addrs[i], 0, sizeof(struct sockaddr_in));
			addrs[i].sin_family = AF_INET;
			addrs[i].sin_port = htons(base_port + 1);
			inet_pton(AF_INET, target_ip, &addrs[i].sin_addr);

			memset(&msgs[i], o, sizeof(struct mmsghdr));
			msgs[i].msg_hdr.msg_name = &addrs[i];
			msg[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
			msg[i].msg_hdr.msg_iov = &iov[i];
			msg[i].msg_hdr.msg_iovlen = 1;
		}

		int sent = sendmmsh(sock, msgs, PORTS_PER_THREAD, 0);
	    printf("[Thread %d] Sent %d UDP packets (ports %d-%d)\n", base_port / PORTS_PER_THREAD, sent, base_port, base_port + PORTS_PER_THREAD - 1);

		close(sock);
		pthread_exit(NULL);
}

	int main(){
		pthread_t threads[THREAD_COUNT];
		int base_ports[THREAD_COUNT];

		for(int i = 0; i < THREAD_COUNT; i++){
			base_ports[i]  = i * PORTS_PER_THREAD;
			pthread_create(&threads[i], NULL, burster, &base_ports[i]);
		}
		for (int i = 0; i < THREAD_COUNT; i++) {
		     pthread_join(threads[i], NULL);
		 }

		 printf("[+] Burst wave complete.\n");
		   return 0;
	}

