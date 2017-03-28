#ifndef SEPA_AGGREGATOR

#ifndef SEPA_PRODUCER
#define SEPA_PRODUCER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define KPI_PRODUCE_FAIL		-1

long kpProduce(const char * update_string,const char * http_server);

#endif // SEPA_PRODUCER

#endif // SEPA_AGGREGATOR
