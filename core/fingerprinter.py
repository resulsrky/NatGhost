# fingerprinter.py
from scapy.all import IP, UDP, send, sr1, ICMP
import random, time

def send_udp_probe(src_port, dst_ip, dst_port):
    pkt = IP(dst=dst_ip)/UDP(sport=src_port, dport=dst_port)/b"FINGERPRINT"
    send(pkt, verbose=False)

def test_nat_behavior(my_ip, target_ip, port_list=[4444, 5555, 6666]):
    print(f"[*] Starting NAT fingerprinting on {target_ip}...")

    results = []

    for port in port_list:
        print(f"[-] Sending probe from src_port={port} → dst_port={port}")
        send_udp_probe(src_port=port, dst_ip=target_ip, dst_port=port)
        time.sleep(1)

        ans = sr1(IP(dst=target_ip)/ICMP(), timeout=2, verbose=False)
        if ans:
            print(f"[+] ICMP reply from {target_ip} → UDP passed NAT")
            results.append((port, True))
        else:
            print(f"[✗] No ICMP reply from {target_ip} → Possibly blocked")
            results.append((port, False))

    print("\n=== NAT Behavior Summary ===")
    for port, success in results:
        print(f"Port {port}: {'✓ PASSED' if success else '✗ BLOCKED'}")

    fingerprint = interpret_results(results)
    print(f"\n🧠 NAT Type Guess: {fingerprint}")

def interpret_results(results):
    passed = [r for _, r in results if r]
    failed = [r for _, r in results if not r]

    if len(passed) == len(results):
        return "Full Cone NAT"
    elif len(passed) == 1 and all(not r for _, r in results[1:]):
        return "Symmetric NAT"
    elif passed and failed:
        return "Restricted or Port Restricted NAT"
    else:
        return "Unknown or Firewall Drop"

if __name__ == "__main__":
    dst = input("Target IP: ")
    test_nat_behavior(my_ip="192.168.1.124", target_ip=dst)

