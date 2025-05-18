# tcp_trigger.py
#Author:Abdurrahman Karadağ
from scapy.all import IP, TCP, send
from time import sleep, time 

def send_tcp_trigger(dst_ip, dst_port=80, src_port=4444, count=10, delay=0, flags='S'):
    """
    Parameters
    ----------
    dst_ip : str
        Target IP Address
    dst_port : int 
        Target port, default is 80.
    src_port : int 
        Source port, default is 4444(MUST be same as UDP).
    count : int
        How many packets going to send. The default is 10.
    delay : float
        Delay. The default is 0.
    flags : str
        TCP flags. The default is 'S'.

    Returns
    -------
    None.

    """
    print(f"[+] Sending TCP trigger packets to {dst_ip}:{dst_port} with flags={flags}")
    start_time = time()
    
    for i in range(count):
        packet = IP(dst=dst_ip)/TCP(sport=src_port, dport=dst_port, flags=flags)
        send(packet, verbose=False)
        print(f"  [#] Packet {i+1}/{count} sent.")
        sleep(delay)
    
    elapsed = time() - start_time
    print(f"[+] Done in {elapsed:.2f} seconds.")

if __name__ == "__main__":
    target = input("Target IP: ")
    send_tcp_trigger(dst_ip=target, dst_port=80, src_port=4444, count=5, delay=0.25, flags="S")
