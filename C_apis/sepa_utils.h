#ifndef SEPA_UTILS_H_INCLUDED
#define SEPA_UTILS_H_INCLUDED

#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _init_HttpJsonResult()      (HttpJsonResult) {.size=0, .json=NULL}

typedef struct http_json_result {
    size_t size;
    char *json;
} HttpJsonResult, *pHttpJsonResult;

int http_client_init();

void http_client_free();

void freeHttpJsonResult(pHttpJsonResult jresult);

size_t queryResultAccumulator(void *ptr,
                              size_t size,
                              size_t nmemb,
                              pHttpJsonResult data);

char * strdup_format(char *origin, const char *format);

#ifdef __cplusplus
}
#endif

#endif // SEPA_UTILS_H_INCLUDED
