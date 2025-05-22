import socket

def start_udp_listener(listen_port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', listen_port))
    print(f"[*] Listening on UPD port {listen_port}.....")

    while True:
        data, addr = sock.recvfrom(4096)
        print(f"[+] Received from {addr}: {data.decode(errors='ignore')}")

if __name__ == "__main__":
    port = int(input("UDP port to listen on:"))
    start_udp_listener(port)
    