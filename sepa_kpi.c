/*
 * sepa_kpi.c
 * 
 * Copyright 2017 Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include "sepa_kpi.h"

int subscription_code_progression = FIRST_SUBSCRIPTION_CODE;
long ws_closed = 0;
int alive_subscriptions = 0;
pthread_mutex_t ws_close_sequentializer;

static int sepa_subscription_callback(	struct libwebsocket_context *context,
										struct libwebsocket *wsi,
										enum libwebsocket_callback_reasons reason,
										void *user, 
										void *in, 
										size_t len) {
	return 0;
}

static const struct libwebsocket_protocols protocols[] = {
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
}



long kpSubscribe(SEPA_subscription_params params) {
	pthread_t subThread;

	subscription_code_progression++;
	params.subscription_code = subscription_code_progression;
	if (pthread_create(&subThread,NULL,subscription_thread,(void *) &params) {
		fprintf(stderr,"Problem in creating subscription thread! Exiting...\n");
		return EXIT_FAILURE;
	}
	
	return params.subscription_code;
}

void * subscription_thread(void * parameters) {
	struct lws_context_creation_info lwsContext_info;
	struct lws_context *context;
	struct lws_client_connect_info connect_info;
	SEPA_subscription_params *params = (SEPA_subscription_params *) parameters;
	
	char *protocol,*p;
	char path[300];
	int use_ssl=-1,m=0,force_exit=0;
	
	memset(&lwsContext_info,0,sizeof lws_info);
	memset(&connect_info,0,sizeof connect_info);
	
	if (lws_parse_uri(parameters->address,&protocol,&(connect_info.address),&(connect_info.port),&p)) {
		fprintf(stderr,"Error while parsing address %s\n",parameters->address);
		return EXIT_FAILURE;
	}
	path[0]='/';
	strncpy(path+1,p,sizeof(path)-2);
	path[sizeof(path)-1]='\0';
	connect_info.path = path;
	
	if (!strcmp(protocol,"ws"))	use_ssl=0;
	if (!strcmp(protocol,"wss")) use_ssl=1;
	if (use_ssl==-1) {
		fprintf(stderr,"Requested protocol %s error: must be ws or wss.\n",protocol);
		return EXIT_FAILURE;
	}
		
	lwsContext_info.port = CONTEXT_PORT_NO_LISTEN;
	lwsContext_info.protocols = protocols;
	lwsContext_info.extensions = NULL;
	lwsContext_info.gid = -1;
	lwsContext_info.uid = -1;
	lwsContext_info.ws_ping_pong_interval = 0;
	context = lws_create_context(&lwsContext_info);
	if (context==NULL) {
		fprintf(stderr,"Creating sepa subscription libwebsocket context failed\n");
		return EXIT_FAILURE;
	}
	
	connect_info.context = context;
	connect_info.ssl_connection = use_ssl;
	connect_info.host = connect_info.address;
	connect_info.origin = connect_info.address;
	connect_info.ietf_version_or_minus_one = -1;
	
	do {
		lws_client_connect_via_info(&connect_info);
		lws_service(context,500);
		pthread_mutex_lock(&ws_close_sequentializer);
		if (ws_closed == parameters->subscription_code) force_exit=1;
		pthread_mutex_unlock(&ws_close_sequentializer);
	} while (!force_exit);
	lws_context_destroy(context);
}

void kpUnsubscribe(long subscription_code) {
	pthread_mutex_lock(&ws_close_sequentializer);
	ws_closed = subscription_code;
	alive_subscriptions--;
	pthread_mutex_unlock(&ws_close_sequentializer);
}

int getAliveSubscriptions() {
	int result;
	pthread_mutex_lock(&ws_close_sequentializer);
	result = alive_subscriptions;
	pthread_mutex_unlock(&ws_close_sequentializer);
	return result;
}

void kpSubscriberInit() {
	subscription_code_progression = FIRST_SUBSCRIPTION_CODE;
	ws_closed = 0;
	alive_subscriptions = 0;
	pthread_mutex_init(&ws_close_sequentializer,NULL);
}

void kpSubscriberClose() {
	pthread_mutex_destroy(&ws_close_sequentializer);
}
