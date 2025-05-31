/*STUN serverlarını belleğe yükle->DNS çözümleme problemi:getaddrinfo
 Hepsine UDP Paketi gönder->Çok sayıda sendto() çağırma problemi:Dizi ile çalış
 Cevapları dinle*>recvfrom() blocking fonksiyondur birinden cevap gelmeden diğerini dinlemez:
 Cevabı hangi sunucunun verdiğini tespit et->Gelen her fd yi karşılaştırmak O(n) zaman alır: epoll kullan
 En hızlı 3 sunucuyu bul:RTT sıralaması linked list ile yapılırsa(default) yavaş, cache-frinedly değil

  Sorun 1:epoll bir event geldiğinde sadece fd'yi döner
  	struct epoll_event ev;
  	ev.data.fd = sock_fd;
  	ama bu şekilde hangi ctx in bu fd yi kullandığını bilmiyoruz.

  Çözüm 1:fd_map[sock_fd] = ctx_id;
  	sock_fd geldiğinde -> fd_map[sock_fd] sana ctx_table[ctx_id]'yi verir
  	Bu sayede O(1) hızda doğrudan bağlam bulunur.
  		Çözüm hashMap yapısından ilham alarak tasarlanmıştır.
  	ctx_table[3]->&ctx_pool[3]
  	ctx_table[3]->sock_fd = 13;

  	fd_map[13] = 3;

  	epoll_wait() -> event.fd = 13
  	->fd_map[13] = 3;
  	->ctx = ctx_table[3];
  	
*/

#include <stdio.h> //printf, perror, fprintf vb. I/O işlemleri
#include <stdlib.h> //malloc, free, exit, rand, getenv vb.
#include <stdint.h> // unint8_t, uint16_t gibi sabit uzunlukta tipler
#include <string.h> //memset, memcpy, strcmp, strncmp vb.
#include <unistd.h> //close, write, usleep, read, fork vb.unix system calls
#include <time.h>
#include <fcntl.h> //open, O_CREATE, O_RDWR vb. file control flags
#include<arpa/inet.h> //inet_ntop, htons, ntohs: IP adresi dönüştürme
#include <sys/socket.h> //socket, sendto,recvfrom vb. soket işlemleri
#include <netinet/in.h> //sockadd_in, IPPROTO_UDP  vb. ağ protokolleri
#include <netdb.h> // getaddribfo gibi hostname çözümleme işlemleri
#include <sys/epoll.h> //epoll_create,epoll_wait: çoklu I/O event handler
#include <sys/types.h> // çeşitli veri tipleri size_t , pid_t vb.
#include <sys/uio.h> //writev gibi birden fazla buffer a aynı anda yazma
#include <sys/mman.h> //mmap, munmap:shared memory
#include <pthread.h> // pthread_t , pthread_create, mutex, thread management

#define MAX_CTX 256
#define MAX_FD 1024 // genellikle epoll max fd

#define MAX_STUN 100
#define BATCH_SIZE 64
#define MAGIC_COOKIE 0x2112A442 
#define STUN_PKT_LEN 20
#define TIME_LIMIT_MS 100
#define SHM_NAME "/natghost_shm" // shared memory name 
#define SHM_SIZE 64 // shame memory size - byte

typedef struct {
	char host[256];
	char ip_str[INET_ADDRSTRLEN]; //IP adresinin string hali(örn "8.8.8.8")
	uint16_t port;
	struct sockaddr_in;//Hedef adres yapısı sento için
	uint8_t transaction_id[12];
	struct timespec send_time;
	long rtt_ms;
	char public_ip[INET_ADDRSTRLEN]; // STUN response dan alınan kendi Public Ip
	uint16_t public_port;
	} stun_ctx_t;

	stun_ctx_t ctx_pool[MAX_CTX];//Memory Pool
	stun_ctx_t *ctx_table[MAX_CTX];//Indexable Access Tablr

	int ctx_table_count = 0;
	int fd_map[MAX_FD]; // fd_map[fd] = ctx_id

	static stun_ctx_t stun_list[MAX_STUN];
	static int stun_count = 0;
	static int sock_fd;
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

	static int compare_rtt(const stun_ctx_t *a, const stun_ctx_t *b){
	if (a->rtt_ms < 0 && b->rtt_ms < 0) return 0;
	if (a->rtt_ms < 0) return 1;
	if (b->rtt_ms < 0) return -1;
	return (a->rtt_ms > b->rtt_ms) - (a->rtt_ms < b->rtt_ms);
	}

	void init_ctx_system(){
		for(int i = 0; i < MAX_FD; i++)
			fd_map[i] = -1;

		ctx_table_count = 0;
	}
	
	void load_stun_servers() {
	    const char *servers[] = {
	        "stun.voipbuster.com", "stun.voipstunt.com", "stun.voipzoom.com",
	        "stun.linphone.org",  "stun.antisip.com",   "stun.gmx.net",
	        "stun.sipnet.net",    "stun.12voip.com",    "stun.3cx.com",
	        "stun.ekiga.net"
	    };
	    int total_size = sizeof(servers)/sizeof(servers[0]);
	    
	    for(int i = 0; i < total_size && stun_count < MAX_STUN;i++){
			struct addinfo hints = {0}, *res;
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_DGRAM;
			if(getaddrinfo(servers[i], "3478", &hints, &res) == 0){
			
				stun_ctx_t *ctx = &ctx_pool[ctx_table_count];
				ctx_table[ctx_table_count++] = ctx;
				
				strncpy(ctx->host, servers[i], sizeof(ctx->host)-1);
				ctx->host[sizeof(ctx->host)-1] = '\0';
				ctx->port = 3478;
				memcpy(&ctx->addr, res->ai_addr, sizeof(ctx->addr));
    			inet_ntop(AF_INET, &ctx->addr.sin_addr, ctx->ip_str, sizeof(ctx->ip_str));

			    ctx->rtt_ms = -1;
			    ctx->public_ip[0] = '\0';
   				ctx->public_port = 0;

   				 freeaddrinfo(res);
   				 stun_count++;
			}
			
		}
}

		int epoll_fd = epoll_create1(0);
		if (epoll_fd < 0) {
		    perror("epoll_create1");
		    exit(EXIT_FAILURE);
		}
		
		for (int i = 0; i < ctx_table_count; i++) {
		    stun_ctx_t *ctx = ctx_table[i];
		
		    // 1️⃣ UDP soket oluştur
		    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
		    if (sock_fd < 0) {
		        perror("socket");
		        continue;
		    }
		
		    // 2️⃣ Test (dummy) veri gönder
		    char dummy = 0;
		    sendto(sock_fd, &dummy, 1, 0,
		           (struct sockaddr *)&ctx->addr, sizeof(ctx->addr));
		
		    // 3️⃣ fd_map eşlemesi
		    fd_map[sock_fd] = i;  // ctx_id = i
		
		    // 4️⃣ epoll kaydı
		    struct epoll_event ev = {0};
		    ev.events = EPOLLIN;          // Veri geldiğinde bildir
		    ev.data.fd = sock_fd;
		
		    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev) < 0) {
		        perror("epoll_ctl");
		        continue;
		    }
		
		    // Soketi ctx içinde saklayalım (ileride close edebilmek için)
		    ctx->sock_fd = sock_fd;
		}

		void listen_for_responses(int epoll_fd, int timeout_ms) {
		    struct epoll_event events[MAX_CTX];
		    int nfds;
		
		    nfds = epoll_wait(epoll_fd, events, MAX_CTX, timeout_ms);
		    if (nfds < 0) {
		        perror("epoll_wait");
		        return;
		    }
		
		    for (int i = 0; i < nfds; i++) {
		        int fd = events[i].data.fd;
		        int ctx_id = fd_map[fd];
		        if (ctx_id < 0 || ctx_id >= ctx_table_count) continue;
		
		        stun_ctx_t *ctx = ctx_table[ctx_id];
		
		        // 🕒 RTT ölçümü: cevap geldiği an zaman al
		        struct timespec recv_time;
		        clock_gettime(CLOCK_MONOTONIC, &recv_time);
		
		        // 📨 Veri oku (STUN cevabı geleceği için en az 20 byte yer ayır)
		        uint8_t buffer[512];
		        socklen_t addrlen = sizeof(struct sockaddr_in);
		        ssize_t len = recvfrom(fd, buffer, sizeof(buffer), 0,
		                               (struct sockaddr *)&ctx->addr, &addrlen);
		
		        if (len <= 0) continue; // cevap alınamadıysa geç
		
		        // RTT hesapla
		        long rtt = (recv_time.tv_sec - ctx->send_time.tv_sec) * 1000 +
		                   (recv_time.tv_nsec - ctx->send_time.tv_nsec) / 1000000;
		
		        ctx->rtt_ms = rtt;
		
		        // ✨ İstersen logla
		        printf("✓ Response from %s (%s): %ld ms\n",
		               ctx->host, ctx->ip_str, ctx->rtt_ms);
		    }
		}
		
		
