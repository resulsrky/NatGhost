# nat_sniffer_v2.py
from scapy.all import sniff, IP, TCP, UDP, ICMP
from collections import defaultdict
from datetime import datetime

# Gözlemlediğimiz her eşleşmeyi burada tutacağız
responses = defaultdict(list)

def log(msg):
    now = datetime.now().strftime("%H:%M:%S")
    print(f"[{now}] {msg}")

def classify_packet(pkt):
    if IP in pkt:
        src = pkt[IP].src
        dst = pkt[IP].dst

        if ICMP in pkt:
            icmp_type = pkt[ICMP].type
            icmp_code = pkt[ICMP].code
            if icmp_type == 3 and icmp_code == 3:
                log(f"ICMP Destination Unreachable from {src} → UDP hedefi muhtemelen ulaştı")
                responses[(src, dst)].append("ICMP_PORT_UNREACHABLE")

        elif UDP in pkt:
            log(f"UDP packet from {src}:{pkt[UDP].sport} → {dst}:{pkt[UDP].dport}")
            responses[(src, dst)].append("UDP_RESPONSE")

        elif TCP in pkt:
            flags = pkt[TCP].flags
            summary = f"TCP {flags} from {src}:{pkt[TCP].sport} → {dst}:{pkt[TCP].dport}"
            if flags == "R":
                log(f"{summary} [RST] → Forge SYN cevabı olabilir")
                responses[(src, dst)].append("TCP_RST")
            elif flags == "RA":
                log(f"{summary} [RST, ACK]")
                responses[(src, dst)].append("TCP_RST_ACK")
            elif flags == "SA":
                log(f"{summary} [SYN-ACK]")
                responses[(src, dst)].append("TCP_SYN_ACK")
            else:
                log(f"{summary}")
                responses[(src, dst)].append(f"TCP_{flags}")

def run_sniffer(timeout=15):
    log(f"NATGhost Passive Analyzer listening for {timeout} seconds...")
    sniff(filter="ip", prn=classify_packet, timeout=timeout, store=0)

    print("\n====== Summary ======")
    for (src, dst), actions in responses.items():
        print(f"{src} → {dst}:")
        for act in actions:
            print(f"   - {act}")
    print("=====================\n")

if __name__ == "__main__":
    run_sniffer(timeout=20)
