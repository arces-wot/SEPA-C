CC=gcc
LDFLAGS=$(shell pkg-config --cflags --libs glib-2.0 libwebsockets libcurl) -pthread

SRC= jsmn.c \
     sepa_utilities.c \
     sepa_consumer.c \
     sepa_producer.c \
     sepa_secure.c
     
HEADERS=$(subst .c,.h,$(SRC))

EXEC= sepaupdate \
      sepaquery \
      sepasubscribe \
      sepatest \

all:
	make sepa-update
	make sepa-query
	make sepa-subscribe
	make tests
	
sepa-update: $(SRC) $(HEADERS)
	$(CC) $(SRC) ./executables/sepaupdate.c $(LDFLAGS) -o sepaupdate
	
sepa-query: $(SRC) $(HEADERS)
	$(CC) $(SRC) ./executables/sepaquery.c $(LDFLAGS) -o sepaquery
	
sepa-subscribe: $(SRC) $(HEADERS)
	$(CC) $(SRC) ./executables/sepasubscribe.c $(LDFLAGS) -o sepasubscribe
	
tests: $(SRC) $(HEADERS)
	$(CC) $(SRC) ./test/sepa-c-test.c $(LDFLAGS) -o sepatest

clean:
	rm -f $(EXEC)
