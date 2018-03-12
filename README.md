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
gcc ./executables/sepaquery.c jsmn.c sepa_producer.c sepa_consumer.c sepa_utilities.c sepa_secure.c -lcurl -pthread `pkg-config --cflags --libs glib-2.0 libwebsockets` -o ./executables/sepaquery
gcc ./executables/sepaupdate.c jsmn.c sepa_producer.c sepa_consumer.c sepa_utilities.c sepa_secure.c -lcurl -pthread `pkg-config --cflags --libs glib-2.0 libwebsockets` -o ./executables/sepaupdate
gcc ./executables/sepasubscribe.c jsmn.c sepa_producer.c sepa_consumer.c sepa_utilities.c sepa_secure.c -lcurl -pthread `pkg-config --cflags --libs glib-2.0 libwebsockets` -o ./executables/sepasubscribe
```
Notice that you can do this with the shortcut
```
cd SEPA-C
eval $(cat ./executables/sepaquery.c | grep gcc)
eval $(cat ./executables/sepaupdate.c | grep gcc)
eval $(cat ./executables/sepasubscribe.c | grep gcc)
```

Use the same 3 files to have some examples in how to write your own program.

# Author
In case, contact [fr4ncidir](https://github.com/fr4ncidir)
