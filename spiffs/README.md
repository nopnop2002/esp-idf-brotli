# spiffs
Demonstration of compressing and decompressing files on SPIFFS.   
This project uses the SPIFFS file system.   

# Installation

```
git clone https://github.com/nopnop2002/esp-idf-brotli
cd esp-idf-brotli
git clone https://github.com/google/brotli components/brotli
cp esp32/CMakeLists.txt components/brotli/
cd spiffs
idf.py build
```

__There is no MENU ITEM where this application is peculiar.__   


# Screen Shot   
```
I (639) MAIN: Initializing SPIFFS
I (669) MAIN: Partition size: total: 354161, used: 1757
I (669) printDirectory: /root d_name=README.txt d_ino=0 fsize=1417 ---> Original file
I (689) COMPRESS: Start
I (689) COMPRESS: param.srcPath=[/root/README.txt]
I (689) COMPRESS: param.dstPath=[/root/README.txt.br]
I (689) COMPRESS: kFileBufferSize=1024
I (739) COMPRESS: CompressFiles ret=1
I (739) COMPRESS: CompressFiles success
I (739) COMPRESS: Finish
I (739) MAIN: comp_result=0
I (759) printDirectory: /root d_name=README.txt d_ino=0 fsize=1417
I (759) printDirectory: /root d_name=README.txt.br d_ino=0 fsize=863 ---> Compressed file
I (769) DECOMPRESS: Start
I (779) DECOMPRESS: param.srcPath=[/root/README.txt.br]
I (779) DECOMPRESS: param.dstPath=[/root/README.txt.txt]
I (779) DECOMPRESS: kFileBufferSize=1024
I (829) DECOMPRESS: CompressFiles ret=1
I (829) DECOMPRESS: CompressFiles success
I (829) DECOMPRESS: Finish
I (829) MAIN: decomp_result=0
I (849) printDirectory: /root d_name=README.txt d_ino=0 fsize=1417
I (849) printDirectory: /root d_name=README.txt.br d_ino=0 fsize=863
I (849) printDirectory: /root d_name=README.txt.txt d_ino=0 fsize=1417 ---> Decompressed file
I (869) main_task: Returned from app_main()
```

