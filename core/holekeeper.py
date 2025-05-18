# holekeeper.py
from scapy.all import IP, UDP, Raw, send
from threading import Thread
import time

def keepalive_udp(dst_ip, dst_port, src_port=4444, interval=5, duration=60):
    """
    NAT bağlantısını açık tutmak için düzenli aralıklarla UDP keepalive gönder.
    """
    start = time.time()
    pkt = IP(dst=dst_ip)/UDP(sport=src_port, dport=dst_port)/Raw(load="KEEP")

    print(f"[*] Holekeeper started for {duration}s → {dst_ip}:{dst_port} every {interval}s")

    while time.time() - start < duration:
        send(pkt, verbose=False)
        print(f"[+] Keepalive sent to {dst_ip}:{dst_port} from port {src_port}")
        time.sleep(interval)

    print("[+] Holekeeper finished.")

# Paralel çalıştırmak için thread bazlı başlatıcı
def start_holekeeper(dst_ip, dst_port, src_port=4444, interval=5, duration=60):
    t = Thread(target=keepalive_udp, args=(dst_ip, dst_port, src_port, interval, duration))
    t.start()
    return t

# Manuel kullanım
if __name__ == "__main__":
    target = input("Hedef IP: ")
    start_holekeeper(dst_ip=target, dst_port=80)

