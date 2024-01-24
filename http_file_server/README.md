# http_file_server   
Demonstration of compression and decompression using HTTP Server.   
This project is based on [this](https://github.com/espressif/esp-idf/tree/master/examples/protocols/http_server/file_serving) example.   

![http-file-server](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/4b388125-c82a-485b-ad6a-079ee8ff786b)

# Installation

```
git clone https://github.com/nopnop2002/esp-idf-brotli
cd esp-idf-brotli/http_file_server
idf.py menuconfig
idf.py flash
```

# Configuration
![config-appi-1](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/4cbba7e5-7765-410c-bf85-dd663556b00c)

You can use staic IP address.   
![config-app-2](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/3ae3ecca-2f9d-47e9-8e87-6ccdd3a02593)

You can connect using the mDNS hostname instead of the IP address.   
![config-app-3](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/8c83734b-0592-4586-8b7c-ee127bba0f8a)


# How to use
ESP32 acts as a web server.   
Enter the following in the address bar of your web browser.   
```
http:://{IP of ESP32}/
or
http://esp32-server.local/
```

Drag one file to drop zone and push Compress button.   


# Partition Table
Define Partition Table in partitions_example.csv.   
When your SoC has 4M Flash, you can expand the partition.   
The total maximum partition size for a SoC with 4M Flash is 0x2F0000(=3,008K).   
storage0 is the storage area for files before compression.   
And storage1 is the storage area for files after compression.   
```
# Name,   Type, SubType, Offset,  Size, Flags
# Note: if you have increased the bootloader size, make sure to update the offsets to avoid overlap
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x190000,
storage0, data, spiffs,  ,        0x30000,
storage1, data, spiffs,  ,        0x30000,
```
