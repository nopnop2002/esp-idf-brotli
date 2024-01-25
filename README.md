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

# Software requiment
- brotli version 1.1.   
 The version of brotli is written [here](https://github.com/google/brotli/blob/master/c/common/version.h#L20).   

- ESP-IDF V4.4/V5.x.   
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

### Installing brotli on Linux
```
sudo apt-get install brotli
```

### Compress file using brotli
```
brotli path_to_input -o path_to_output
```

### Decompress file using brotli
```
brotli -d path_to_input -o path_to_output
```

### Example
```
brotli README.md -o README.md.br
brotli -d  README.md.br -o README.md.md
diff README.md README.md.md
```

# How to use brotli on python
```
python3 -m pip install Brotli
cd esp-idf-brotli/python

# Compress file
python3 compress.py path_to_compress path_to_output

# Decompress file
python3 decompress.py path_to_decompress path_to_output
```


### Example
```
python3 compress.py test.txt test.txt.br
python3 decompress.py test.txt.br test.txt.txt
diff test.txt test.txt.txt
```

# Comparison of zlib and brotli

|source file|source size(byte)|brotli comress(byte)|zlib compress(byte)|
|:-:|:-:|:-:|:-:|
|test.txt|20479|9470|4571|
|esp32.jpeg|18753|18613|18218|
|esp32.png|43540|43640|43264|

# Reference
https://github.com/nopnop2002/esp-idf-zlib
