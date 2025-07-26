cmake_minimum_required(VERSION 3.10)
project(natghost_forge C)
set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(
        natghost_forge
        src/main.c
        src/network_core.c
        src/checksum.c
)

# Windows için Winsock2 kütüphanesini bağla
target_link_libraries(natghost_forge PRIVATE ws2_32)