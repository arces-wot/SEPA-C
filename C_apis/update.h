#ifndef UPDATE_H_INCLUDED
#define UPDATE_H_INCLUDED

#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "sepa_secure.h"
#include "sepa_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UPDATE_RESULT_BUFFER_LENGTH       512


#define sepa_update_unsecure(sparql, host, result)              sepa_update(sparql, host, NULL, NULL, NULL, NULL, result)
#define sepa_update_secure(sparql, host, id, jwt, result)       sepa_update(sparql, host, NULL, NULL, id, jwt, result)
#define sepa_clear_unsecure(host, result)                       sepa_update("delete where {?a ?b ?c}", host, NULL, NULL, NULL, NULL, result)
#define sepa_clear_secure(host, id, jwt, result)                sepa_update("delete where {?a ?b ?c}", host, NULL, NULL, id, jwt, result)

long sepa_update(const char *sparql_update,
                 const char *host,
                 const char *registration_request_url,
                 const char *token_request_url,
                 char *client_id,
                 psClient jwt,
                 pHttpJsonResult result_data);

#ifdef __cplusplus
}
#endif

#endif // UPDATE_H_INCLUDED
