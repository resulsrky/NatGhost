import mmap
import os
import time

SHM_NAME = "/natghost_shm"
SHM_SIZE = 64  # C tarafında 64 byte ayırmıştık

def read_shared_memory():
    fd = os.open(f"/dev/shm{SHM_NAME}", os.O_RDONLY)
    with mmap.mmap(fd, SHM_SIZE, access=mmap.ACCESS_READ) as m:
        data = m.read(SHM_SIZE).split(b'\x00', 1)[0].decode()
    os.close(fd)
    return data

if __name__ == "__main__":
    time.sleep(0.1)  # C'nin yazması tamamlanana kadar küçük gecikme
    ip_port = read_shared_memory()
    print(f"[Python] Public IP and Port from SHM: {ip_port}")
