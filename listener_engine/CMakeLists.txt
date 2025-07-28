# CMake'in minimum versiyonunu ve proje adını belirtiyoruz.
cmake_minimum_required(VERSION 3.10)
project(listener_engine C)
set(CMAKE_C_STANDARD 11)

# --- Npcap Kütüphanesini Bulma (En Güvenilir Yöntem) ---
# Npcap SDK'nın senin bilgisayarındaki tam yolunu bir değişkene atıyoruz.
set(NPCAP_SDK_PATH "C:/Program Files/npcap-sdk-1.15")

# Bu klasörün var olup olmadığını kontrol ediyoruz.
if(NOT EXISTS "${NPCAP_SDK_PATH}")
    message(FATAL_ERROR "Npcap SDK su yolda bulunamadi: ${NPCAP_SDK_PATH}\nLutfen Npcap'i 'Install the Npcap SDK' secenegi isaretli olarak kurdugunuzdan emin olun.")
endif()

# Npcap başlık dosyalarının olduğu klasörü DOĞRUDAN ekliyoruz.
include_directories(${NPCAP_SDK_PATH}/Include)

# HATA ÇÖZÜMÜ: Linker'a kütüphanenin tam yolunu veriyoruz.
# Önce 64-bit klasörünü, sonra 32-bit klasörünü kontrol ediyoruz.
if(EXISTS "${NPCAP_SDK_PATH}/Lib/x64/wpcap.lib")
    set(PCAP_LIBRARY_PATH "${NPCAP_SDK_PATH}/Lib/x64/wpcap.lib")
    message(STATUS "64-bit Npcap kütüphanesi bulundu.")
elseif(EXISTS "${NPCAP_SDK_PATH}/Lib/wpcap.lib")
    set(PCAP_LIBRARY_PATH "${NPCAP_SDK_PATH}/Lib/wpcap.lib")
    message(STATUS "32-bit Npcap kütüphanesi bulundu.")
else()
    message(FATAL_ERROR "wpcap.lib kütüphanesi Npcap SDK klasöründe bulunamadi!")
endif()

# --- Proje Yapılandırması ---
# Kendi başlık dosyalarımızın olduğu klasörü ekliyoruz.
include_directories(include)

# Çalıştırılabilir dosyayı ve kaynak dosyalarını tanımlıyoruz.
add_executable(
        listener_engine
        src/main.c
)

# Gerekli kütüphaneleri programımıza bağlıyoruz.
# Artık sadece kütüphane adını değil, tam yolunu veriyoruz.
target_link_libraries(listener_engine PRIVATE ws2_32 "${PCAP_LIBRARY_PATH}")
