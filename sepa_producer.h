#ifndef SEPA_PRODUCER
#define SEPA_PRODUCER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define KPI_PRODUCE_FAIL		-1
#define HTTP					0
#define HTTPS					1

long kpProduce(const char * update_string,const char * http_server);

#endif // SEPA_PRODUCER
