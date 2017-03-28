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

SEPA_subscriber sepa_session;
pSEPA_subscriber p_sepa_session = NULL;

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
			p_sepa_session = &sepa_session;
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
	int result=EXIT_FAILURE,first_run;
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
			if (lws_parse_uri(params->address,&protocol,&(connect_info.address),&(connect_info.port),&p)) {
				fprintf(stderr,"Error while parsing address %s\n",params->address);
				pthread_exit(EXIT_FAILURE);
			}
			if (pthread_create(&(params->subscription_task),NULL,subscription_thread,(void *) params) {
				subscription_task = NULL;
				fprintf(stderr,"Problem in creating subscription thread! Exiting...\n");
				result = EXIT_FAILURE;
			}
		}
		pthread_mutex_unlock(&(sepa_session.subscription_mutex)));
	}
	return result;
}

int kpUnsubscribe(int subscription_code,pthread_t * subscription_task) {
	int result=EXIT_FAILURE,i;
	if (!pthread_mutex_lock(&(sepa_session.subscription_mutex))) {
		sepa_session.closing_subscription = subscription_code;
		pthread_mutex_unlock(&(sepa_session.subscription_mutex));
		
		pthread_join(subscription_task,NULL);
	}
	if (!pthread_mutex_lock(&(sepa_session.subscription_mutex))) {
		sepa_session.closing_subscription = 0;		
		i=0;
		while (i<sepa_session.active_subscriptions) {
			if (sepa_session.subscription_codes[i]==subscription_code) {
				if (i<sepa_session.active_subscriptions-1) {
					sepa_session.subscription_codes[i] = sepa_session.subscription_codes[sepa_session.active_subscriptions-1];
					qsort(sepa_session.subscription_codes,sepa_session.active_subscriptions-1,sizeof(int),compare_integer);
				}
				result = EXIT_SUCCESS;
				if (sepa_session.active_subscriptions==1) {
					free(sepa_session.subscription_codes);
					sepa_session.subscription_codes = NULL;
				}
				else {
					sepa_session.subscription_codes = (int *) realloc(sepa_session.subscription_codes,(sepa_session.active_subscriptions-1)*sizeof(int));
					if (sepa_session.subscription_codes==NULL) {
						fprintf(stderr,"Realloc error in kpUnsubscribe\n");
						result = EXIT_FAILURE;
					}
				}
				i=sepa_session.active_subscriptions;
			}
			else i++;
		}
		sepa_session.active_subscriptions--;
		pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	}
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
	struct lws_context *context;
	struct lws_client_connect_info connect_info;
	pSEPA_subscription_params params = (pSEPA_subscription_params) parameters;
	char *protocol,*p;
	char path[300];
	int use_ssl=-1,m=0,force_exit=0;
	
	memset(&lwsContext_info,0,sizeof lws_info);
	memset(&connect_info,0,sizeof connect_info);
	
	
	path[0]='/';
	strncpy(path+1,p,sizeof(path)-2);
	path[sizeof(path)-1]='\0';
	connect_info.path = path;
	
	if (!strcmp(protocol,"ws"))	use_ssl=0;
	if (!strcmp(protocol,"wss")) use_ssl=1;
	if (use_ssl==-1) {
		fprintf(stderr,"Requested protocol %s error: must be ws or wss.\n",protocol);
		pthread_exit(EXIT_FAILURE);
	}
	
	
}
