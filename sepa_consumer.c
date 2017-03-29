/*
 * sepa_consumer.c
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


#include "sepa_consumer.h"

volatile SEPA_subscriber sepa_session;

static int sepa_subscription_callback(	struct libwebsocket_context *context,
										struct libwebsocket *wsi,
										enum libwebsocket_callback_reasons reason,
										void *user, 
										void *in, 
										size_t len) {
	return 0;
}

int compare_integer(const void * a, const void * b) {
	const int *ia = (const int *) a;
	const int *ib = (const int *) b;
	return (*ia>*ib)-(*ia<*ib); 
}

int sepa_subscriber_init() {
	int lockResult,result=EXIT_FAILURE;
	lockResult = pthread_mutex_lock(&(sepa_session.subscription_mutex));
	switch (lockResult) {
		case EINVAL:
			sepa_session.closing_subscription = 0;
			sepa_session.active_subscriptions = 0;
			sepa_session.subscription_codes = NULL;
			pthread_mutex_init(&(sepa_session.subscription_mutex),NULL);
			result = EXIT_SUCCESS;
			break;
		case 0:
			pthread_mutex_unlock(&(sepa_session.subscription_mutex));
			break;
		default:
			perror("Error in sepa_subscriber_init - ");
			break;
	}
	return result;
}

int sepa_subscriber_destroy() {
	if (!getActiveSubscriptions()) {
		pthread_mutex_destroy(&(sepa_session.subscription_mutex));
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int kpSubscribe(pSEPA_subscription_params params) {
	int result = EXIT_FAILURE;
	char *p;
	if ((params!=NULL) && (!pthread_mutex_lock(&(sepa_session.subscription_mutex)))) {
		sepa_session.active_subscriptions++;
		if (sepa_session.active_subscriptions==1) {
			sepa_session.subscription_codes = (int *) malloc(sizeof(int));
			if (sepa_session.subscription_codes==NULL) fprintf(stderr,"Malloc error in kpSubscribe.\n");
			else {
				params->subscription_code = 1;
				sepa_session.subscription_codes = 1;
				result = EXIT_SUCCESS;
			}
		}
		else {
			sepa_session.subscription_codes = (int *) realloc(sepa_session.subscription_codes,sepa_session.active_subscriptions*sizeof(int));
			if (sepa_session.subscription_codes==NULL) fprintf(stderr,"Realloc error in kpSubscribe.\n");
			else {
				params->subscription_code = sepa_session.subscription_codes[sepa_session.active_subscriptions-2]+1;
				sepa_session.subscription_codes[sepa_session.active_subscriptions-1]=params->subscription_code;
				result = EXIT_SUCCESS;
			}
		}
		if (result==EXIT_SUCCESS) {
			if (lws_parse_uri(params->server_address,&(params->protocol),&(params->address),&(params->port),&p)) {
				fprintf(stderr,"Error while parsing address %s\n",params->server_address);
				result = EXIT_FAILURE;
			}
			else {
				(params->path)[0]='/';
				strncpy(params->path+1,p,sizeof(params->path)-2);
				path[sizeof(params->path)-1]='\0';
				
				params->use_ssl = -1;
				if (!strcmp(params->protocol,"ws"))	params->use_ssl=0;
				if (!strcmp(params->protocol,"wss")) params->use_ssl=1;
				if (params->use_ssl==-1) {
					fprintf(stderr,"Requested protocol %s error: must be ws or wss.\n",params->protocol);
					result = EXIT_FAILURE;
				}
				else {
					if (pthread_create(&(params->subscription_task),NULL,subscription_thread,(void *) params) {
						subscription_task = NULL;
						fprintf(stderr,"Problem in creating subscription thread! Exiting...\n");
						result = EXIT_FAILURE;
					}
				}
			}
		}
		pthread_mutex_unlock(&(sepa_session.subscription_mutex)));
	}
	return result;
}

int kpUnsubscribe(pSEPA_subscription_params params) {
	int result=EXIT_FAILURE,i=0,code_index=-1;
	
	if (!pthread_mutex_lock(&(sepa_session.subscription_mutex))) {
		while ((i<sepa_session.active_subscriptions) && (code_index==-1)) {
			if (sepa_session.subscription_codes[i]==params->subscription_code) {
				code_index = i;
			}
			else i++;
		}
		if (i==sepa_session.active_subscriptions) {
			if (!i) fprintf(stderr,"No active subscriptions: cannot unsubscribe.\n");
			else fprintf(stderr,"Nonexistent subscription code %d\n",params->subscription_code);
			pthread_mutex_unlock(&(sepa_session.subscription_mutex));
			return EXIT_FAILURE;
		}
		else pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	}
	
	pthread_mutex_lock(&(sepa_session.subscription_mutex));
	sepa_session.closing_subscription = subscription_code;
	pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	pthread_join(params->subscription_task,NULL);

	pthread_mutex_lock(&(sepa_session.subscription_mutex));
	sepa_session.closing_subscription = 0;
	sepa_session.active_subscriptions--;
	result = EXIT_SUCCESS;
	if (!sepa_session.active_subscriptions) {
		free(sepa_session.subscription_codes);
		sepa_session.subscription_codes = NULL;
	}
	else {		
		for (i=code_index; i<sepa_session.active_subscriptions; i++) {
			sepa_session.subscription_codes[i]=sepa_session.subscription_codes[i+1];
		}
		sepa_session.subscription_codes = (int *) realloc(sepa_session.subscription_codes,sepa_session.active_subscriptions*sizeof(int));
		if (sepa_session.subscription_codes==NULL) {
			fprintf(stderr,"Realloc error in kpUnsubscribe\n");
			result = EXIT_FAILURE;
		}
	}
	pthread_mutex_unlock(&(sepa_session.subscription_mutex));

	return result;
}

int getActiveSubscriptions() {
	int result=-1;
	if (!pthread_mutex_lock(&(sepa_session.subscription_mutex))) {
		result = sepa_session.active_subscriptions;
		pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	}
	return result;
}


void * subscription_thread(void * parameters) {
	struct lws_context_creation_info lwsContext_info;
	struct lws_context *ws_context;
	struct lws_client_connect_info connect_info;
	pSEPA_subscription_params params = (pSEPA_subscription_params) parameters;
	int force_exit = 0;
	
	memset(&lwsContext_info,0,sizeof lws_info);
	memset(&connect_info,0,sizeof connect_info);
	
	lwsContext_info.port = CONTEXT_PORT_NO_LISTEN;
	lwsContext_info.protocols = _getSubscriptionProtocols();
	lwsContext_info.extensions = NULL;
	lwsContext_info.gid = -1;
	lwsContext_info.uid = -1;
	lwsContext_info.ws_ping_pong_interval = 0;
	ws_context = lws_create_context(&lwsContext_info);
	if (ws_context==NULL) {
		fprintf(stderr,"Creating sepa subscription libwebsocket context failed\n");
		pthread_exit(EXIT_FAILURE);
	}
	
	connect_info.context = ws_context;
	connect_info.address = params->address;
	connect_info.port = *(params->port);
	connect_info.path = params->path;
	connect_info.protocol = _getSubscriptionProtocolName();
	connect_info.ssl_connection = params->use_ssl;
	connect_info.host = connect_info.address;
	connect_info.origin = connect_info.address;
	connect_info.ietf_version_or_minus_one = -1;
	
	while (!force_exit) {
		lws_client_connect_via_info(&connect_info);
		
		lws_service(ws_context, 500);
		
		pthread_mutex_lock(&(sepa_session.subscription_mutex));
		force_exit = (params->subscription_code==sepa_session.closing_subscription);
		pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	}
	pthread_exit(EXIT_SUCCESS);
}
