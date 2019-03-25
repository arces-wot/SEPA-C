#ifndef SUBSCRIPTION_H_INCLUDED
#define SUBSCRIPTION_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <libwebsockets.h>
#include "jsmn.h"
#include "sepa_secure.h"
#include "sepa_utils.h"

#define MAX_WAITING_CHANNELS    5
#define RX_BUFFER_SIZE          100
#define LWS_PIPE_CHOKED_TIMEOUT	4.0 /**< Max seconds (float) available for websocket pipe freeing */
#define CHUNK_MAX_SIZE          64 /**< Default dimension of websocket output buffer */
#define FIRST_NOTIFICATION      0
#define LATER_NOTIFICATION      1
#define UNSUBSCRIBE             -1
#define ERROR_EXPLORING_RESULT  -2



typedef void * (*SubscriptionHandler)(void *);
typedef struct ws_channel SubscriptionChannel, *pSubscriptionChannel;

typedef struct subscription {
    SubscriptionHandler handler;
    char *sparql;
    char *alias;
    char *spuid;
    pSubscriptionChannel channel;
} Subscription, *pSubscription;

struct ws_channel {
    char *host;
    int n_sub;
    pSubscription subs;
    pthread_mutex_t ws_mutex;
    pthread_cond_t connected;
    struct lws *ws_id;
    psClient jwt;
    char *r_buffer;
};

#define _init_subscription()                                            (Subscription) {.handler=NULL, .sparql=NULL, .alias=NULL, .spuid=NULL}

#define new_unsecure_channel(host, n_sub, channel)                      open_subscription_channel(host, n_sub, NULL, NULL, NULL, NULL, channel)
#define new_secure_channel(host, n_sub, id, jwt, channel)               open_subscription_channel(host, n_sub, NULL, NULL, id, jwt, channel)

#define sepa_subscribe_secure(sparql, alias, handler, channel)          sepa_subscribe(sparql, alias, NULL, NULL, handler, channel)
#define sepa_subscribe_unsecure(sparql, alias, handler, channel)        sepa_subscribe(sparql, alias, NULL, NULL, handler, channel)

int open_subscription_channel(const char *host,
                              int n_sub,
                              const char *registration_request_url,
                              const char *token_request_url,
                              char *client_id,
                              psClient jwt,
                              pSubscriptionChannel channel);

pSubscription sepa_subscribe(const char *sparql,
                             const char *alias,
                             char *d_graph,
                             char *n_graph,
                             SubscriptionHandler handler,
                             pSubscriptionChannel channel);

int sepa_unsubscribe(pSubscription subscription);

void freeSubscription(pSubscription sub);

void close_subscription_channel(pSubscriptionChannel channel);

#ifdef __cplusplus
}
#endif

#endif // SUBSCRIPTION_H_INCLUDED
