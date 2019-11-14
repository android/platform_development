# CPP2C
This is simple tool to generate a C interface from C++ source code using clang libtooling. CPP2C currently is a proof of concept, 
and only applies to [https://github.com/samanbarghi/uThreads](https://github.com/samanbarghi/uThreads), to generate the C interface. The code is explained in details in [this blog post](http://samanbarghi.com/blog/2016/12/06/generate-c-interface-from-c-source-code-using-clang-libtooling/).

## Building and Installation
```
git clone https://github.com/samanbarghi/CPP2C
cd CPP2C
mkdir build
cmake ..
make
sudo make install
```

## Run it over uThreads source code
```
git clone https://github.com/samanbarghi/uThreads
cd uThreads
cpp2c include/uThreads.h -- -I./src -I/usr/include/x86_64-linux-gnu/c++/5/ -I/usr/include/c++/5.4.0 -std=c++11
```

## Output
You can find the output in _outpu_ folder: _cwrapper.h_ is the header that should be included in C files, and _cwrapper.cpp_ is in C++ and is responsible to convert C++ pointers to C and vice versa. 
