# C apis for SEPA 0.8.4

# Installing
Prerequisites:
```
sudo apt-get install cmake libssl-dev libcurl4-gnutls-dev libglib2.0-dev
```

Install [LibWebsockets](https://github.com/warmcat/libwebsockets):
```
git clone https://github.com/warmcat/libwebsockets.git
cd ./libwebsockets
mkdir build
cd build
cmake ..
make
sudo make install
sudo ldconfig
```

Compile executables examples:
```
cd SEPA-C
make
```

and to clean up...
```
cd SEPA-C
make clean
```

Use the following 3 files to have some examples in how to write your own program.
```
./executables/sepaquery.c
./executables/sepaupdate.c
./executables/sepasubscribe.c
```
Documentation is coming...

# Testing
After make, you can also run some tests
```
./sepatest [sepa-ip-address]
```
Where `sepa-ip-address` is someting like 192.168.1.100, without protocols and ports (we consider the standards written in [SPARQL 1.1 SE Protocol](http://wot.arces.unibo.it/TR/sparql11-se-protocol.html).

# Author
In case, contact [fr4ncidir](https://github.com/fr4ncidir)
