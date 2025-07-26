#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <natghost/config.h>
#include <natghost/network_core.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define PROCESS_COUNT 16

void run_master(const char *self_path, const char *source_ip, const char *dest_ip);
void run_worker_from_args(int argc, char *argv[]);

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup basarisiz oldu.\n");
        return 1;
    }

    // Argüman kontrolü: Program argümanla mı çağrıldı, argümansız mı?
    if (argc > 1) {
        // Argüman varsa bu bir İŞÇİ process'tir.
        run_worker_from_args(argc, argv);
    } else {
        // Argüman yoksa bu ANA process'tir.
        run_master(argv[0], "192.168.1.153", "8.8.8.8");
    }

    WSACleanup();
#else
    printf("Bu kod sadece Windows icin tasarlanmistir.\n");
#endif
    return 0;
}

void run_master(const char *self_path, const char *source_ip, const char *dest_ip) {
    printf("NATGhost Packet Crafter - MULTI-PROCESS BURST MODE\n");

    uint16_t total_ports_to_scan = 65535;
    int ports_per_process = total_ports_to_scan / PROCESS_COUNT;

    PROCESS_INFORMATION pi[PROCESS_COUNT] = {0};
    HANDLE process_handles[PROCESS_COUNT] = {0};

    printf("Burst baslatiliyor -> Hedef: %s, Portlar: 1-%u, Process Sayisi: %d\n", dest_ip, total_ports_to_scan, PROCESS_COUNT);

    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    for (int i = 0; i < PROCESS_COUNT; i++) {
        char command_line[256];
        uint16_t start_port = (i * ports_per_process) + 1;
        uint16_t end_port = (i == PROCESS_COUNT - 1) ? total_ports_to_scan : (i + 1) * ports_per_process;

        sprintf(command_line, "\"%s\" %s %s %u %u", self_path, source_ip, dest_ip, start_port, end_port);

        STARTUPINFOA si; // ASCII versiyonunu kullan
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi[i], sizeof(pi[i]));

        if (!CreateProcessA(NULL, command_line, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi[i])) {
            fprintf(stderr, "HATA: Process %d olusturulamadi: %lu\n", i + 1, GetLastError());
            process_handles[i] = NULL;
        } else {
            process_handles[i] = pi[i].hProcess;
            CloseHandle(pi[i].hThread);
        }
    }

    WaitForMultipleObjects(PROCESS_COUNT, process_handles, TRUE, INFINITE);

    QueryPerformanceCounter(&end);
    double interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;

    printf("\n[+] Tum process'ler tamamlandi. Burst dalgasi bitti.\n");
    printf("Toplam sure: %.4f saniye\n", interval);
    if (interval > 0) {
        printf("Paket gonderim hizi: %.2f pps (paket/saniye)\n", (double)total_ports_to_scan / interval);
    }

    for (int i = 0; i < PROCESS_COUNT; i++) {
        if(process_handles[i]) {
            CloseHandle(process_handles[i]);
        }
    }
}

void run_worker_from_args(int argc, char *argv[]) {
    if (argc != 5) {
        return;
    }
    const char* source_ip = argv[1];
    const char* dest_ip = argv[2];
    uint16_t start_port = atoi(argv[3]);
    uint16_t end_port = atoi(argv[4]);

    run_worker(source_ip, dest_ip, start_port, end_port);
}