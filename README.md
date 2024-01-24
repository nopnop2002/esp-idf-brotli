# esp-idf-brotli
Google brotli for ESP-IDF.   

Brotli is a generic-purpose lossless compression algorithm that compresses data using a combination of a modern variant of the LZ77 algorithm, Huffman coding and 2nd order context modeling, with a compression ratio comparable to the best currently available general-purpose compression methods. It is similar in speed with deflate but offers more dense compression.   
Homepage is [here](https://github.com/google/brotli/tree/master).   

I ported [this](https://github.com/google/brotli/blob/master/c/tools/brotli.c) for ESP-IDF.   

# Installation overview

1. In the components directory, clone Brotli version 1.1:
```
git clone https://github.com/google/brotli components/brotli
```

2. In the new Brotli directory, create a CMakeLists.txt file.
```
cp esp32/CMakeLists.txt components/brotli/
```

3. Compile this project.


# Software requirements
ESP-IDF V4.4/V5.x.   
ESP-IDF V5.0 is required when using ESP32-C2.   
ESP-IDF V5.1 is required when using ESP32-C6.   

# Installation
```
git clone https://github.com/nopnop2002/esp-idf-brotli
cd esp-idf-brotli
git clone https://github.com/google/brotli components/brotli
cp esp32/CMakeLists.txt components/brotli/
cd spiffs
idf.py build
```

# How to use brotli on Linux
```
$ sudo apt-get install brotli

# Compress
$ brotli test.txt

# Decompress
$ brotli -d test.txt.br -o test2.txt
```


# Comparison of zlib and brotli

|source file|source size(byte)|brotli comress(byte)|zlib compress(byte)|
|:-:|:-:|:-:|:-:|
|test.txt|20479|9470|4571|
|esp32.jpeg|18753|18613|18218|
|esp32.png|43540|43640|43264|

# Reference
https://github.com/nopnop2002/esp-idf-zlib
