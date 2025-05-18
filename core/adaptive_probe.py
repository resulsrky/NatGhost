# adaptive_probe.py
from scapy.all import IP, TCP, send
from threading import Thread
from time import sleep
import random

flags_set = ["S", "F", "A", "R", "SA", "FA", "PA", "RA"]

def forge_and_send(dst_ip, dst_port, src_port, src_ip, flags):
    """
    Forge edilmiş TCP paketini gönder.
    """
    pkt = IP(src=src_ip, dst=dst_ip)/TCP(sport=src_port, dport=dst_port, flags=flags)
    send(pkt, verbose=False)
    print(f"[+] Sent {flags} from {src_ip}:{src_port} → {dst_ip}:{dst_port}")

def adaptive_probe(dst_ip, dst_port=80, spoofed_ips=None, src_ports=None, delay=0.05):
    """
    TCP bayrakları + zombi IP'lerle forge edilmiş adaptive probe gönderimi
    """
    if spoofed_ips is None:
        spoofed_ips = ["192.168.1.101", "192.168.1.102", "192.168.1.103"]
    if src_ports is None:
        src_ports = [4444, 5555, 6666]

    threads = []

    print(f"[*] Starting adaptive probing to {dst_ip}:{dst_port}...")

    for flag in flags_set:
        for src_ip in spoofed_ips:
            for src_port in src_ports:
                t = Thread(target=forge_and_send, args=(dst_ip, dst_port, src_port, src_ip, flag))
                t.start()
                threads.append(t)
                sleep(delay)

    for t in threads:
        t.join()

    print("[+] Probing completed. Run nat_sniffer.py to analyze responses.")

# Eğer direkt çalıştırılırsa
if __name__ == "__main__":
    target = input("Target IP: ")
    adaptive_probe(dst_ip=target, dst_port=80)

