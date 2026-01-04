# mqtt
Demonstration of compression using MQTT.   
I found [this](http://www.steves-internet-guide.com/send-file-mqtt/) article.   
It's great to be able to exchange files using MQTT.   
Since the python code was publicly available, I ported it to esp-idf.   

# Installation

```
git clone https://github.com/nopnop2002/esp-idf-brotli
cd esp-idf-brotli/mqtt
idf.py menuconfig
idf.py flash
```

# Configuration   

![config-top](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/bf29fa8a-c71c-4bc7-a2c9-9f7801e0d7b4)
![config-app](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/79d80b54-c4da-428b-aca2-e78f7ebb3c8d)

## WiFi Setting
Set the information of your access point.   

![config-wifi](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/ea4a100a-72f0-44a1-b4c4-a5cc4218d3d1)

## Broker Setting

MQTT broker is specified by one of the following.
- IP address   
 ```192.168.10.20```   
- mDNS host name   
 ```mqtt-broker.local```   
- Fully Qualified Domain Name   
 ```broker.emqx.io```

You can download the MQTT broker from [here](https://github.com/nopnop2002/esp-idf-mqtt-broker).   

![config-broker-1](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/a2010b57-0f4a-4e8f-b110-cdcda2c767a5)

Specifies the username and password if the server requires a password when connecting.   
[Here's](https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-debian-10) how to install and secure the Mosquitto MQTT messaging broker on Debian 10.   

![config-broker-2](https://github.com/nopnop2002/esp-idf-brotli/assets/6020549/85f8d05a-5d85-4985-8c21-c5a038466b15)

# How to use

Run the following python script on the host side.
```
$ python3 -m pip install paho-mqtt

Default Broker is broker.emqx.io.   
You can specify a different broker when starting the script.

$ python3 mqtt-file.py sdkconfig [broker]
- Send sdkconfig to ESP32 using mqtt
- Compress sdkconfig using brotli
- Receiving compressed files from ESP32 using mqtt
- Delete source files from ESP32
- Delete compressed files from ESP32

$ ls -l sdkconfig
-rw-rw-r-- 1 nop nop 67241 Jan  4 11:49 sdkconfig

$ ls -l sdkconfig.br
-rw-rw-r-- 1 nop nop 27973 Jan  4 11:53 sdkconfig.br
```

# MQTT Topic
This project uses the following topics:
```
MQTT_PUT_REQUEST="/mqtt/files/put/req"
MQTT_GET_REQUEST="/mqtt/files/get/req"
MQTT_LIST_REQUEST="/mqtt/files/list/req"
MQTT_DELETE_REQUEST="/mqtt/files/delete/req"

MQTT_PUT_RESPONSE="/mqtt/files/put/res"
MQTT_GET_RESPONSE="/mqtt/files/get/res"
MQTT_LIST_RESPONSE="/mqtt/files/list/res"
MQTT_DELETE_RESPONSE="/mqtt/files/delete/res"
```

When using public brokers, these topics may be used for other purposes.   
If you want to change the topic to your own, you will need to change both the ESP32 side and the python side.   
You can use [this](https://github.com/nopnop2002/esp-idf-mqtt-broker) as your personal Broker.   
