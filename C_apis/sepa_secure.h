#ifndef SECURE_H_INCLUDED
#define SECURE_H_INCLUDED

#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include "sepa_utils.h"
#include "jsmn.h"
#include "base64.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REGISTRATION_BUFFER_LENGTH      1024
#define _init_sClient()     {.client_id=NULL, .client_secret=NULL, .JWT=NULL, .JWTtype=NULL, .expiration=0 }

typedef struct secure_client_data {
    char *client_id;
    char *client_secret;
    char *JWT;
    char *JWTtype;
    long expiration;
} sClient, *psClient;

#define printf_sClient(jwt)     fprintf_sClient(stdout, jwt)
void fprintf_sClient(FILE *destination, sClient jwt);
int store_sClient(const char *path, sClient jwt);
int read_sClient(const char *path, psClient jwt);

void sClient_free(psClient jwt);

long sepa_register(const char *client_id,
                   const char *registration_request_url,
                   psClient jwt);

long sepa_request_token(const char *token_request_url,
                        psClient jwt);
#ifdef __cplusplus
}
#endif

#endif // SECURE_H_INCLUDED
