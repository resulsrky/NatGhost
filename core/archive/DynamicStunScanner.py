"""
DynamicStunScanner v1.0
Author: Abdurrahman & GPT-4
Description:
  Aynı anda aşağıdaki işlemleri gerçekleştiren dinamik STUN sunucu tarayıcısı:
  - DNS SRV taraması (_stun._udp.<domain>)
  - A/AAAA kaydı çözümleme
  - Ters DNS brute-force (alt alan adları üinden)
"""


import dns.resolver
import dns.reversename
import dns.exception
import socket
from multiprocessing import Process, Manager

COMMON_DOMAINS = [
    "google.com", "cloudflare.com", "twilio.com", "stunprotocol.org", "voipbuster.com",
    "sipnet.ru", "netgsm.com.tr", "zoiper.com", "ekiga.net", "linphone.org",
    "voipcheap.com", "justvoip.com", "gmx.net", "1und1.de", "stun.freeswitch.org"
]

COMMON_SUBDOMAINS = ["stun", "turn", "media", "relay", "ice", "sip", "sip1", "sip2"]

found = Manager().list()

def resolve_srv(domain):
    try:
        answers = dns.resolver.resolve(f"_stun._udp.{domain}", "SRV")
        for rdata in answers:
            target = str(rdata.target).rstrip('.')
            port = rdata.port
            found.append((target, port))
    except dns.exception.DNSException:
        pass

def resolve_a(domain):
    try:
        answers = dns.resolver.resolve(domain, "A")
        for rdata in answers:
            found.append((domain, 3478))
    except dns.exception.DNSException:
        pass

def resolve_aaaa(domain):
    try:
        answers = dns.resolver.resolve(domain, "AAAA")
        for rdata in answers:
            found.append((domain, 3478))
    except dns.exception.DNSException:
        pass

def reverse_brute(domain):
    for sub in COMMON_SUBDOMAINS:
        full = f"{sub}.{domain}"
        resolve_a(full)
        resolve_aaaa(full)
        resolve_srv(full)

def run_all():
    jobs = []
    for domain in COMMON_DOMAINS:
        jobs.append(Process(target=resolve_srv, args=(domain,)))
        jobs.append(Process(target=resolve_a, args=(domain,)))
        jobs.append(Process(target=resolve_aaaa, args=(domain,)))
        jobs.append(Process(target=reverse_brute, args=(domain,)))

    for j in jobs:
        j.start()
    for j in jobs:
        j.join()

    return list(set(found))  # duplicate'leri temizle

if __name__ == "__main__":
    servers = run_all()
    print("\n=== DYNAMICALLY FOUND STUN SERVERS ===")
    for s in servers:
        print(f"> {s[0]}:{s[1]}")
