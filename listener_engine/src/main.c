#define WIN32_LEAN_AND_MEAN
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h> // ntohs gibi fonksiyonlar için
#include "net_headers.h"

// Yakalanan her paket için Npcap tarafından çağrılacak olan ana analiz fonksiyonu
void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
    // Ethernet başlığı 14 byte'tır. IP başlığı ondan sonra başlar.
    const natghost_iphdr_t *iph = (natghost_iphdr_t *)(packet + 14);

    // Sadece UDP paketleriyle ilgileniyoruz çünkü bu, başarılı bir delik açma anlamına gelir.
    if (iph->protocol == IPPROTO_UDP) {
        int ip_header_length = iph->ihl * 4;
        const natghost_udphdr_t *udph = (natghost_udphdr_t *)(packet + 14 + ip_header_length);

        // Gelen paketin kaynak portu, bizim delmeyi başardığımız porttur.
        uint16_t open_port = ntohs(udph->source);

        // Ekrana sadece başarılı bir şekilde açılan ortak portu (deliği) yazdır.
        printf("[BASARILI] Ortak delik acildi! Karsi tarafin portu: %u\n", open_port);
    }
    // ICMP "Port Unreachable" mesajlarını (kapalı portları) görmezden geliyoruz.
}

int main(int argc, char *argv[]) {
    pcap_if_t *alldevs, *d;
    char errbuf[PCAP_ERRBUF_SIZE];
    int i = 0;
    int dev_num;

    // Komut satırı argümanını kontrol et
    if (argc != 2) {
        fprintf(stderr, "Kullanim: %s <dinlenecek_hedef_ip>\n", argv[0]);
        fprintf(stderr, "Ornek: %s 192.168.1.15\n", argv[0]);
        return 1;
    }
    const char* target_ip = argv[1];

    // 1. Tüm ağ arayüzlerini bul
    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1) {
        fprintf(stderr, "HATA: Arayuzler bulunamadi: %s\n", errbuf);
        return 1;
    }

    // 2. Arayüzleri listele ve kullanıcıdan seçim yapmasını iste
    printf("Kullanilabilir ag arayuzleri:\n");
    for (d = alldevs; d; d = d->next) {
        printf("%d. %s", ++i, d->name);
        if (d->description) printf(" (%s)\n", d->description);
        else printf(" (Aciklama yok)\n");
    }

    if (i == 0) {
        printf("\nHic arayuz bulunamadi! Npcap surucusunun kurulu oldugundan emin olun.\n");
        pcap_freealldevs(alldevs);
        return 1;
    }

    printf("Dinlenecek arayuzun numarasini girin (1-%d): ", i);
    if (scanf("%d", &dev_num) != 1) {
        printf("Gecersiz giris.\n");
        pcap_freealldevs(alldevs);
        return 1;
    }

    if (dev_num < 1 || dev_num > i) {
        printf("Gecersiz secim.\n");
        pcap_freealldevs(alldevs);
        return 1;
    }

    // Seçilen arayüze git
    for (d = alldevs, i = 0; i < dev_num - 1; d = d->next, i++);

    // 3. Seçilen cihazı dinlemek için aç (promiscuous mode aktif)
    pcap_t *handle = pcap_open_live(d->name, 65536, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "HATA: Arayuz acilamadi (%s): %s\n", d->name, errbuf);
        pcap_freealldevs(alldevs);
        return 1;
    }

    // 4. Filtreyi oluştur ve uygula. Sadece belirtilen hedeften gelen UDP paketlerini yakala.
    struct bpf_program fp;
    char filter_exp[128];
    sprintf(filter_exp, "src host %s and udp", target_ip);

    if (pcap_compile(handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        fprintf(stderr, "HATA: Filtre derlenemedi: %s\n", pcap_geterr(handle));
        pcap_freealldevs(alldevs);
        return 1;
    }

    if (pcap_setfilter(handle, &fp) == -1) {
        fprintf(stderr, "HATA: Filtre ayarlanamadi: %s\n", pcap_geterr(handle));
        pcap_freealldevs(alldevs);
        return 1;
    }

    printf("\n[+] %s arayuzu dinleniyor...\n[+] Filtre: \"%s\"\n[+] Ortak delikler bekleniyor...\n", d->description, filter_exp);
    pcap_freealldevs(alldevs);

    // 5. Paket yakalama döngüsünü başlat (Ctrl+C ile durdurulabilir)
    pcap_loop(handle, 0, packet_handler, NULL);

    pcap_close(handle);
    printf("\n[+] Dinleme durduruldu.\n");

    return 0;
}
