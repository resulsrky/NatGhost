# udp_flooder.py
from scapy.all import IP, UDP, Raw, send
from time import sleep, time
# read_shm.py
import os
import mmap

SHM_NAME = "/natghost_shm"
SHM_PATH = f"/dev/shm{SHM_NAME}"
SHM_SIZE = 64

def read_ip_port():
    """
    Shared memory'den 'IP:PORT' formatlı stringi okuyup
    (ip:str, port:int) tuple olarak döner.
    Eğer bellek boşsa (':0') ('', 0) döner.
    """
    # Shared memory segmentini aç
    fd = os.open(SHM_PATH, os.O_RDONLY)
    try:
        # Belleği map'le
        with mmap.mmap(fd, SHM_SIZE, access=mmap.ACCESS_READ) as m:
            raw = m.read(SHM_SIZE).split(b'\x00', 1)[0].decode()
    finally:
        os.close(fd)

    if ':' not in raw:
        return '', 0
    ip, port_str = raw.split(':', 1)
    try:
        port = int(port_str)
    except ValueError:
        port = 0
    return ip, port

def send_udp_flood(dst_ip, dst_port, src_port=4444, count=20, delay=0.1, payload="PING"):

    print(f"[+] Sending UDP packets to {dst_ip}:{dst_port} from port {src_port}")
    start_time = time()

    for i in range(count):
        packet = IP(dst=dst_ip)/UDP(sport=src_port, dport=dst_port)/Raw(load=payload)
        send(packet, verbose=False)
        print(f"  [#] UDP Packet {i+1}/{count} sent.")
        sleep(delay)
    
    elapsed = time() - start_time
    print(f"[+] UDP flood done in {elapsed:.2f} seconds.")
    
# Doğrudan çalıştırma örneği
if __name__ == "__main__":
    target = input("Target IP: ")
    port = input("\nTarget Port: ")
    send_udp_flood(dst_ip=target, dst_port=port, src_port=4444, count=10, delay=0.1, payload="hello_natsniff")
