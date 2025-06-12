# flutter_software_embedder

This project is an example flutter software embedder which renders Flutter application **inside a terminal** using Sixel graphics 🎯

This project provides a **custom Flutter embedder** that runs Flutter engine output directly in terminals supporting sixel, allowing full Flutter UI inside a terminal window (e.g., *xterm*, *rxvt*) with sixel-enabled VTE implementations.


[![video](https://img.youtube.com/vi/H-sHaze6DTA/0.jpg)](https://youtu.be/H-sHaze6DTA)

**(Click image for video preview)**

### How to build

```
mkdir build
cd build
cmake ..
make
```

### How to run

```
./flutter_software ../hello_world <path to your engine>/src/flutter/third_party/icu/common/icudtl.dat
```

