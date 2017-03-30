#ifndef SEPA_AGGREGATOR

#ifndef SEPA_CONSUMER
#define SEPA_CONSUMER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "/usr/local/include/libwebsockets.h"

#define PATH_ADDRESS_LEN					100
#define PROTOCOL_LEN						100
#define WS_ADDRESS_LEN						100
#define _getSubscriptionProtocols()			_protocols
#define _getSubscriptionProtocolName()		_protocols[0].name
#define _getSubscriptionCallback()			_protocols[0].sepa_subscription_callback
#define _getSubscriptionDataSize()			_protocols[0].per_session_data_size
#define _getSubscriptionRxBufferSize()		_protocols[0].rx_buffer_size

typedef struct subscription_params {
	char *subscription_sparql;
	
	int use_ssl;
	int subscription_code;
	pthread_t subscription_task;
	char protocol[PROTOCOL_LEN];
	char address[WS_ADDRESS_LEN];
	char path[PATH_ADDRESS_LEN];
	int port;
	struct lws *ws_identifier;
} SEPA_subscription_params,*pSEPA_subscription_params;

typedef struct sepa_subscriber {
	int active_subscriptions;
	int * subscription_codes;
	pthread_mutex_t subscription_mutex;
	int closing_subscription;
} SEPA_subscriber,*pSEPA_subscriber;

int sepa_subscriber_init();
int sepa_subscriber_destroy();
int getActiveSubscriptions();
int sepa_subscription_builder(char * sparql_subscription,char * server_address,pSEPA_subscription_params subscription);
int kpSubscribe(pSEPA_subscription_params params);
int kpUnsubscribe(pSEPA_subscription_params params);
void * subscription_thread(void * parameters);



static int sepa_subscription_callback(	struct lws *wsi,
										enum lws_callback_reasons reason,
										void *user, 
										void *in, 
										size_t len);

#endif // SEPA_CONSUMER

#endif // SEPA_AGGREGATOR
