#ifndef SEPA_AGGREGATOR

#ifndef SEPA_CONSUMER
#define SEPA_CONSUMER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libwebsockets.h>

typedef struct subscription_params {
	char *subscription_string;
	char *server_address;
	int use_ssl;
	
	int subscription_code;
	pthread_t subscription_task;
	char *protocol;
	char *address;
	char *path;
	int *port;
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
int kpSubscribe(SEPA_subscription_params params,pthread_t * subscription_task);
int kpUnsubscribe(int subscription_code);

#endif // SEPA_CONSUMER

#endif // SEPA_AGGREGATOR
