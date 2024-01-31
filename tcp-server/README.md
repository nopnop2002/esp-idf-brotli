# tcp-server   
Demonstration of compression and decompression using TCP Server.   
This project is based on [this](https://github.com/espressif/esp-idf/tree/master/examples/protocols/sockets/tcp_server) example.   

# Installation

```
git clone https://github.com/nopnop2002/esp-idf-brotli
cd esp-idf-brotli/tcp-server
idf.py menuconfig
idf.py flash
```

# Configuration
![config-top](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/ba295b22-0f5c-414f-ade3-514489f68846)
![config-app-1](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/119bde5e-4bd4-48d2-805e-6df9d932b706)

You can connect using the mDNS hostname instead of the IP address.   
![config-app-2](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/cb0649c4-5ab6-4da2-ae69-07872280ec95)


# How to use
ESP32 acts as a tcp server.   
```
$ python3 tcp-client.py path_to_host [port]
- Send source file to ESP32 using TCP/IP
- Compress source file using brotli
- Receiving compressed files from ESP32 using TCP/IP
- Delete source files from ESP32
- Delete compressed files from ESP32

$ ls -l *.br
```
