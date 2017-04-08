#ifndef SEPA_CONSUMER
#define SEPA_CONSUMER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include "/usr/local/include/libwebsockets.h"
#include "sepa_utilities.h"

#define PATH_ADDRESS_LEN					100
#define PROTOCOL_LEN						100
#define WS_ADDRESS_LEN						100
#define RX_BUFFER_SIZE						100
#define SUBSCRIPTION_TAG					"subscribe="
#define UNSUBSCRIBE_TAG						"unsubscribe="
#define UNSUBSCRIBE_TAG_LEN					12
#define _getSubscriptionProtocols()			_protocols
#define _getSubscriptionProtocolName()		_protocols[0].name
#define _getSubscriptionCallback()			_protocols[0].sepa_subscription_callback
#define _getSubscriptionDataSize()			_protocols[0].per_session_data_size
#define _getSubscriptionRxBufferSize()		_protocols[0].rx_buffer_size
#define _initSubscription()					{.use_ssl=-1,.subscription_code=-1,.identifier="",.protocol="",.address="",.path="",.port=-1}

typedef void (*SubscriptionHandler)(sepaNode *,int,sepaNode *,int);
typedef void (*UnsubscriptionHandler)(void);

typedef struct subscription_params {
	char *subscription_sparql;
	
	int use_ssl;
	int subscription_code;
	char identifier[IDENTIFIER_ARRAY_LEN];
	pthread_t subscription_task;
	char protocol[PROTOCOL_LEN];
	char address[WS_ADDRESS_LEN];
	char path[PATH_ADDRESS_LEN];
	int port;
	struct lws *ws_identifier;
	SubscriptionHandler subHandler;
	UnsubscriptionHandler unsubHandler;
	char *resultBuffer;
} SEPA_subscription_params,*pSEPA_subscription_params;

typedef struct sepa_subscriber {
	int active_subscriptions;
	pSEPA_subscription_params *subscription_list;
	pthread_mutex_t subscription_mutex;
	int closing_subscription_code;
} SEPA_subscriber,*pSEPA_subscriber;

int sepa_subscriber_init();
int sepa_subscriber_destroy();
pSEPA_subscription_params * getSubscriptionList();
int getActiveSubscriptions();
void fprintfSubscriptionParams(FILE * outputstream,SEPA_subscription_params params);
void sepa_setSubscriptionHandlers(SubscriptionHandler subhandler,UnsubscriptionHandler unsubhandler,pSEPA_subscription_params subscription);
int sepa_subscription_builder(char * sparql_subscription,char * server_address,pSEPA_subscription_params subscription);
int kpSubscribe(pSEPA_subscription_params params);
int kpUnsubscribe(pSEPA_subscription_params params);
char * kpQuery(const char * sparql_query,const char * sparql_address);

#endif // SEPA_CONSUMER
