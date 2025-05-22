import dns.resolver
import dns.exception
from multiprocessing import Process, Manager

def query_srv(domain, output):
    srv_types = [
        ('_stun._udp.', 'UDP'),
        ('_stuns._tcp.', 'TCP (TLS)'),
        ('_stun._tcp.', 'TCP')
    ]
    for prefix, proto in srv_types:
        try:
            full_name = prefix + domain
            answers = dns.resolver.resolve(full_name, 'SRV')
            for rdata in answers:
                output.append((str(rdata.target).rstrip('.'), rdata.port, proto, 'SRV'))
        except:
            continue

def query_a(domain, output):
    targets = [f"stun.{domain}", domain]
    for name in targets:
        try:
            answers = dns.resolver.resolve(name, 'A')
            for rdata in answers:
                output.append((rdata.address, 3478, 'UDP', 'A'))
        except:
            continue

def query_aaaa(domain, output):
    targets = [f"stun.{domain}", domain]
    for name in targets:
        try:
            answers = dns.resolver.resolve(name, 'AAAA')
            for rdata in answers:
                output.append((rdata.address, 3478, 'UDP', 'AAAA'))
        except:
            continue

def parallel_dns_discover(domain):
    with Manager() as manager:
        output = manager.list()

        p1 = Process(target=query_srv, args=(domain, output))
        p2 = Process(target=query_a, args=(domain, output))
        p3 = Process(target=query_aaaa, args=(domain, output))

        p1.start()
        p2.start()
        p3.start()

        p1.join()
        p2.join()
        p3.join()

        return list(output)


# === Test ===
if __name__ == "__main__":
    domain = "google.com"
    servers = parallel_dns_discover(domain)

    if servers:
        print("Discovered STUN Servers:")
        for addr, port, proto, record in servers:
            print(f"> {addr}:{port} ({proto}) [{record}]")
    else:
        print("[!] Hiçbir STUN server keşfedilemedi.")
