#ifndef SEPA_AGGREGATOR

#ifndef SEPA_CONSUMER
#define SEPA_CONSUMER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libwebsockets.h>

#define PATH_ADDRESS_LEN					100
#define _getSubscriptionProtocols()			protocols
#define _getSubscriptionProtocolName()		protocols[0].name
#define _getSubscriptionCallback()			protocols[0].sepa_subscription_callback
#define _getSubscriptionDataSize()			protocols[0].per_session_data_size
#define _getSubscriptionRxBufferSize()		protocols[0].rx_buffer_size

typedef struct subscription_params {
	char *subscription_string;
	char *server_address;
	int use_ssl;
	
	int subscription_code;
	pthread_t subscription_task;
	char *protocol;
	char *address;
	char path[PATH_ADDRESS_LEN];
	int *port;
	struct lws *ws_identifier;
} SEPA_subscription_params,*pSEPA_subscription_params;

typedef struct sepa_subscriber {
	int active_subscriptions;
	int * subscription_codes;
	pthread_mutex_t subscription_mutex;
	int closing_subscription;
} SEPA_subscriber,*pSEPA_subscriber;

const struct lws_protocols protocols[] = {
	{
		"SEPA_SUBSCRIPTION",
		sepa_subscription_callback,
		0,
		20
	},
	{
		NULL,
		NULL,
		0,
		0
	}
};

int sepa_subscriber_init();
int sepa_subscriber_destroy();
int getActiveSubscriptions();
int kpSubscribe(SEPA_subscription_params params);
int kpUnsubscribe(pSEPA_subscription_params params);

#endif // SEPA_CONSUMER

#endif // SEPA_AGGREGATOR
