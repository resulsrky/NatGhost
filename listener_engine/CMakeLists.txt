# CMake'in minimum versiyonunu ve proje adını belirtiyoruz.
cmake_minimum_required(VERSION 3.10)
project(listener_engine C)
set(CMAKE_C_STANDARD 11)

# --- Npcap Kütüphanesini Bulma (Hata Düzeltilmiş Hali) ---
# Npcap SDK'nın kurulabileceği olası yolları doğrudan listeliyoruz.
# Ortam değişkenleri bazı sistemlerde sorun çıkarabildiği için bu yöntem daha güvenilirdir.
set(NPCAP_SDK_POSSIBLE_PATHS
        "C:/Program Files/npcap-sdk-1.15"  # Senin bilgisayarındaki onaylanmış yol
        "C:/Program Files/Npcap-SDK"
        "C:/Program Files (x86)/Npcap-SDK"
)

# pcap.h başlık dosyasını bu yollarda arıyoruz.
find_path(PCAP_INCLUDE_DIR pcap.h
        PATHS ${NPCAP_SDK_POSSIBLE_PATHS}
        PATH_SUFFIXES Include # Klasör adı 'Include' (büyük harfle)
)

# wpcap.lib kütüphane dosyasını bu yollarda arıyoruz.
find_library(PCAP_LIBRARY NAMES wpcap
        PATHS ${NPCAP_SDK_POSSIBLE_PATHS}
        PATH_SUFFIXES Lib/x64 Lib # 64-bit ve 32-bit için
)

# Eğer başlık dosyası veya kütüphane bulunamazsa, kullanıcıya ne yapacağını söyleyen bir hata ver ve dur.
if(NOT PCAP_INCLUDE_DIR OR NOT PCAP_LIBRARY)
    message(FATAL_ERROR "Npcap SDK bulunamadi! Lutfen Npcap'i 'Install the Npcap SDK' secenegi isaretli olarak kurdugunuzdan emin olun. Kurulum yolu genellikle 'C:/Program Files/npcap-sdk-...' seklindedir.")
endif()

message(STATUS "Npcap SDK bulundu: ${PCAP_INCLUDE_DIR}")

# --- Proje Yapılandırması ---
# Kendi başlık dosyalarımızın olduğu klasörü ekliyoruz.
include_directories(include)
# Bulunan Npcap başlık dosyalarının olduğu klasörü ekliyoruz.
include_directories(${PCAP_INCLUDE_DIR})

# Çalıştırılabilir dosyayı ve kaynak dosyalarını tanımlıyoruz.
add_executable(
        listener_engine
        src/main.c
)

# Gerekli kütüphaneleri programımıza bağlıyoruz.
target_link_libraries(listener_engine PRIVATE ws2_32 ${PCAP_LIBRARY})
