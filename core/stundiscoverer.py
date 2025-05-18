# StunDiscover.py
import socket
import time
import threading

stun_servers = [
    ("stun.l.google.com", 19302),
    ("stun1.l.google.com", 19302),
    ("stun2.l.google.com", 19302),
    ("stun3.l.google.com", 19302),
    ("stun4.l.google.com", 19302)
]

results = []

def send_probe(server):
    ip, port = server
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(10)
        message = str(time.time()).encode()  # Basit bir timestamp göndereceğiz
        start = time.time()
        sock.sendto(message, (ip, port))
        data, addr = sock.recvfrom(1024)
        end = time.time()
        rtt = (end - start) * 1000  # ms cinsinden RTT
        received_ts = float(data.decode(errors='ignore').strip())

        results.append({
            "server": ip,
            "port": port,
            "rtt": round(rtt, 10),
            "response_timestamp": received_ts
        })
        sock.close()
    except Exception as e:
        pass  # Zaman aşımı, bağlantı yok vs.

threads = []

for server in stun_servers:
    t = threading.Thread(target=send_probe, args=(server,))
    t.start()
    threads.append(t)

for t in threads:
    t.join()

# En hızlı 3 STUN server
sorted_results = sorted(results, key=lambda x: x["rtt"])
best = sorted_results[:3]

print("\n=== STUN Discovery Results ===")
for entry in best:
    print(f"{entry['server']}:{entry['port']} → RTT: {entry['rtt']} ms | ServerTime: {entry['response_timestamp']}")

if best:
    selected = best[0]
    print(f"\n✅ Selected STUN Server → {selected['server']}:{selected['port']}")
else:
    print("\n❌ No STUN server responded.")


