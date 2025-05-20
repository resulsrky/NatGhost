# udp_flooder.py
from scapy.all import IP, UDP, Raw, send
from time import sleep, time

def send_udp_flood(dst_ip, dst_port=80, src_port=4444, count=20, delay=0.1, payload="PING"):
    """
    Aynı source port ve IP ile hedefe forge UDP paketleri gönderir
    
    Args:
        dst_ip (str): Target IP Address
        dst_port (int): Targer Port Address
        src_port (int):Same source port (must be the same as TCP)
        count (int): Kaç adet UDP paketi gönderilecek
        delay (float):  (waiting time between packets(seconds)
        payload (str): UDP content payload
    """
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
    send_udp_flood(dst_ip=target, dst_port=80, src_port=4444, count=10, delay=0.1, payload="hello_natsniff")
