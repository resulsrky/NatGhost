# StunDiscover.py
import socket 
from scapy.all import *
import struct 
from multiprocessing import Process, Queue
import threading
from concurrent.futures import ThreadPoolExecutor
import time 
import random

stun_servers = [
    ("stun.1und1.de", 3478),
    ("stun.gmx.net", 3478),
    ("stun.stunprotocol.org", 3478),
    ("stun.sipnet.net", 3478),
    ("stun.sipnet.ru", 3478),
    ("stun.12connect.com", 3478),
    ("stun.12voip.com", 3478),
    ("stun.3cx.com", 3478),
    ("stun.acrobits.cz", 3478),
    ("stun.actionvoip.com", 3478),
    ("stun.advfn.com", 3478),
    ("stun.altar.com.pl", 3478),
    ("stun.antisip.com", 3478),
    ("stun.avigora.fr", 3478),
    ("stun.bluesip.net", 3478),
    ("stun.cablenet-as.net", 3478),
    ("stun.callromania.ro", 3478),
    ("stun.callwithus.com", 3478),
    ("stun.cheapvoip.com", 3478),
    ("stun.cloopen.com", 3478),
    ("stun.commpeak.com", 3478),
    ("stun.cope.es", 3478),
    ("stun.counterpath.com", 3478),
    ("stun.counterpath.net", 3478),
    ("stun.dcalling.de", 3478),
    ("stun.demos.ru", 3478),
    ("stun.dus.net", 3478),
    ("stun.easycall.pl", 3478),
    ("stun.easyvoip.com", 3478),
    ("stun.ekiga.net", 3478),
    ("stun.epygi.com", 3478),
    ("stun.etoilediese.fr", 3478),
    ("stun.faktortel.com.au", 3478),
    ("stun.freecall.com", 3478),
    ("stun.freeswitch.org", 3478),
    ("stun.freevoipdeal.com", 3478),
    ("stun.gmx.de", 3478),
    ("stun.gradwell.com", 3478),
    ("stun.halonet.pl", 3478),
    ("stun.hellonanu.com", 3478),
    ("stun.hoiio.com", 3478),
    ("stun.hosteurope.de", 3478),
    ("stun.ideasip.com", 3478),
    ("stun.infra.net", 3478),
    ("stun.internetcalls.com", 3478),
    ("stun.intervoip.com", 3478),
    ("stun.ipcomms.net", 3478),
    ("stun.ipfire.org", 3478),
    ("stun.ippi.fr", 3478),
    ("stun.ipshka.com", 3478),
    ("stun.irian.at", 3478),
    ("stun.it1.hr", 3478),
    ("stun.ivao.aero", 3478),
    ("stun.jumblo.com", 3478),
    ("stun.justvoip.com", 3478),
    ("stun.linphone.org", 3478),
    ("stun.liveo.fr", 3478),
    ("stun.lowratevoip.com", 3478),
    ("stun.lundimatin.fr", 3478),
    ("stun.mit.de", 3478),
    ("stun.miwifi.com", 3478),
    ("stun.modulus.gr", 3478),
    ("stun.myvoiptraffic.com", 3478),
    ("stun.netappel.com", 3478),
    ("stun.netgsm.com.tr", 3478),
    ("stun.nfon.net", 3478),
    ("stun.nonoh.net", 3478),
    ("stun.nottingham.ac.uk", 3478),
    ("stun.ooma.com", 3478),
    ("stun.ozekiphone.com", 3478),
    ("stun.pjsip.org", 3478),
    ("stun.poivy.com", 3478),
    ("stun.powervoip.com", 3478),
    ("stun.ppdi.com", 3478),
    ("stun.qq.com", 3478),
    ("stun.rackco.com", 3478),
    ("stun.rockenstein.de", 3478),
    ("stun.rolmail.net", 3478),
    ("stun.roundsapp.com", 3478),
    ("stun.rynga.com", 3478),
    ("stun.schlund.de", 3478),
    ("stun.sipdiscount.com", 3478),
    ("stun.sipgate.net", 3478),
    ("stun.sipgate.net", 10000),
    ("stun.siplogin.de", 3478),
    ("stun.sipnet.net", 3478),
    ("stun.sipnet.ru", 3478),
    ("stun.sippeer.dk", 3478),
    ("stun.sipphone.com", 3478),
    ("stun.siptraffic.com", 3478),
    ("stun.smartvoip.com", 3478),
    ("stun.smsdiscount.com", 3478),
    ("stun.snafu.de", 3478),
    ("stun.softjoys.com", 3478),
    ("stun.solcon.nl", 3478),
    ("stun.sonetel.com", 3478),
    ("stun.sonetel.net", 3478),
    ("stun.sovtest.ru", 3478),
    ("stun.speedy.com.ar", 3478),
    ("stun.spokn.com", 3478),
    ("stun.srce.hr", 3478),
    ("stun.stunprotocol.org", 3478),
    ("stun.symform.com", 3478),
    ("stun.symplicity.com", 3478),
    ("stun.sysadminman.net", 3478),
    ("stun.t-online.de", 3478),
    ("stun.tiscali.com", 3478),
    ("stun.tng.de", 3478),
    ("stun.twt.it", 3478),
    ("stun.u-blox.com", 3478),
    ("stun.ucallweconn.net", 3478),
    ("stun.ucsb.edu", 3478),
    ("stun.ucw.cz", 3478),
    ("stun.ulsan.ac.kr", 3478),
    ("stun.unseen.is", 3478),
    ("stun.usfamily.net", 3478),
    ("stun.viva.gr", 3478),
    ("stun.vivox.com", 3478),
    ("stun.vo.lu", 3478),
    ("stun.vodafone.ro", 3478),
    ("stun.voicetrading.com", 3478),
    ("stun.voip.aebc.com", 3478),
    ("stun.voip.blackberry.com", 3478),
    ("stun.voip.eutelia.it", 3478),
    ("stun.voiparound.com", 3478),
    ("stun.voipbuster.com", 3478),
    ("stun.voipbusterpro.com", 3478),
    ("stun.voipcheap.co.uk", 3478),
    ("stun.voipcheap.com", 3478),
    ("stun.voipfibre.com", 3478),
    ("stun.voipgate.com", 3478),
    ("stun.voipinfocenter.com", 3478),
    ("stun.voipplanet.nl", 3478),
    ("stun.voippro.com", 3478),
    ("stun.voipraider.com", 3478),
    ("stun.voipstunt.com", 3478),
    ("stun.voipwise.com", 3478),
    ("stun.voipzoom.com", 3478),
    ("stun.vopium.com", 3478),
    ("stun.voys.nl", 3478),
    ("stun.voztele.com", 3478),
    ("stun.webcalldirect.com", 3478),
    ("stun.wifirst.net", 3478),
    ("stun.wirefree.in", 3478),
    ("stun.wirelessmoves.com", 3478),
    ("stun.xs4all.nl", 3478),
    ("stun.zadarma.com", 3478),
    ("stun.zadv.com", 3478),
    ("stun.zoiper.com", 3478)
]



results = []
bullet_funcs = []



class ParallelVectorEngine:
    def __init__(self, func, data, max_workers=None):
        """
        func: Her eleman için çağrılacak işlem fonksiyonu
        data: List, tuple veya np.array gibi erişilebilir veri kümesi
        max_workers: Kaç paralel işlem oluşturulacağı (varsayılan tüm veriler kadar)
        """
        self.func = func
        self.data = data
        self.max_workers = max_workers or len(data)
        self.queue = Queue()

    def _worker(self, idx, item):
        """Her bir işlemde çağrılacak işlev"""
        result = self.func(idx, item)
        self.queue.put((idx, result))

    def run(self):
        processes = []

        for idx, item in enumerate(self.data):
            p = Process(target=self._worker, args=(idx, item))
            processes.append(p)
            if len(processes) == self.max_workers:
                self._launch_and_clear(processes)

        # Kalanları da başlat
        if processes:
            self._launch_and_clear(processes)

        # Sonuçları sırayla topla
        results = [None] * len(self.data)
        while not self.queue.empty():
            idx, val = self.queue.get()
            results[idx] = val

        return results

    def _launch_and_clear(self, processes):
        for p in processes:
            p.start()
        for p in processes:
            p.join()
        processes.clear()


"""
MUST be UDP Packet For Binding Request
MUST start with a 20-byte header followed by zero or more Attributes
MUST contains:
>STUN Message Type 
>Magic Cookie
>Transaction ID
>Message Length
*The most significant bit 2 bits of every STUN Message MUST be zeroes.


"""  
def random_ID():
    return bytes([random.randint(0, 255) for _ in range(12)])

def forgeStunPacket(ip):
    m_type = 0x0001 #H->Unsignet Short ->2 byte
    m_length = 0 #H->Unsignet Short ->2 byte 
    magic_cookie = 0x2112A442 #I->Unsignet Int -> 4 byte
    transaction_ID = random_ID() # 12 byte 
    
    stunHeader = struct.pack("!HHI12s", m_type, m_length,magic_cookie, transaction_ID)
   #stunPacket = IP(dst=target)/UDP(dport=3478)/Raw(load=stunHeader)
   #return send(stunPacket)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(3)
    
    start_time = time.time()
    sock.sendto(stunHeader, (ip, 3478))
    try:
        start_time = time.time()
        sock.sendto(stunHeader, (ip, 3478))
        data, _ = sock.recvfrom(1024)
        end_time = time.time()
    except socket.timeout:
        print(f"[×] Timeout: {ip}")
        return
    finally:
        sock.close()
        
    rtt = round((end_time - start_time) * 1000, 2)
    
    #Look for a transactionID
    if data[0:2] == b'\x01\x01' and data[4:8] == b'\x21\x12\xa4\x42':
            results.append({
                'ip': ip,
                'port': 3478,
                'rtt': rtt,
                'timestamp': end_time
            })
            print(f"[✓] Reply from {ip}:3478 → RTT = {rtt} ms")

    else:
            print(f"[!] Invalid STUN reply from {ip}")

    sock.close()

 
def reload_and_fire():
    ips = [ip for ip, _ in stun_servers]

    engine = ParallelVectorEngine(lambda i, ip: forgeStunPacket(ip), ips, max_workers=60)
    engine.run()
    
def discover():
    reload_and_fire()
    sorted_results = sorted(results, key=lambda x: x['rtt'])
    print("\n=== BEST 3 STUN SERVERS ===")
    for r in sorted_results[:3]:
        print(f"> {r['ip']}:{r['port']} | RTT: {r['rtt']} ms")

if __name__ == "__main__":
    discover()
    