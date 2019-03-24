#ifndef QUERY_H_INCLUDED
#define QUERY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "sepa_utils.h"
#include "sepa_secure.h"

#define QUERY_START_BUFFER_LENGTH       512


#define sepa_query_unsecure(sparql, host, result)               sepa_query(sparql, host, NULL, NULL, NULL, NULL, result)
#define sepa_query_secure(sparql, host, id, jwt, result)        sepa_query(sparql, host, NULL, NULL, id, jwt, result)
#define sepa_query_all_unsecure(host, result)                   sepa_query("select * where {?a ?b ?c}", host, NULL, NULL, NULL, NULL, result)
#define sepa_query_all_secure(host, id, jwt, result)            sepa_query("select * where {?a ?b ?c}", host, NULL, NULL, id, jwt, result)

long sepa_query(const char *sparql_query,
                const char *host,
                const char *registration_request_url,
                const char *token_request_url,
                const char *client_id,
                psClient jwt,
                pHttpJsonResult result_data);

#ifdef __cplusplus
}
#endif

#endif // QUERY_H_INCLUDED
