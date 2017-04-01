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

static int exit_success = EXIT_SUCCESS;
static int exit_failure = EXIT_FAILURE;

static void * subscription_thread(void * parameters);
static int sepa_subscription_callback(	struct lws *wsi,
										enum lws_callback_reasons reason,
										void *user, 
										void *in, 
										size_t len);

static const struct lws_protocols _protocols[2] = {
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

static void callbackRaisedUnsubscription(struct lws *wsi) {
	int i=0,wsi_index=-1;
	pthread_mutex_lock(&(sepa_session.subscription_mutex));
	while ((i<sepa_session.active_subscriptions) && (wsi_index==-1)) {
		if (wsi==(sepa_session.subscription_list[i]).ws_identifier) {
			wsi_index==i;
		}
		i++;
	}
	pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	if (wsi_index>0) kpUnsubscribe(&(sepa_session.subscription_list[wsi_index]));
}

static int sepa_subscription_callback(	struct lws *wsi,
										enum lws_callback_reasons reason,
										void *user, 
										void *in, 
										size_t len) {
	
	switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Sepa Callback: Connect with server success.\n");
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("Sepa Callback: Connect with server error.\n");
            callbackRaisedUnsubscription(wsi);
            break;
        case LWS_CALLBACK_CLOSED:
            printf("Sepa Callback: LWS_CALLBACK_CLOSED\n");
            callbackRaisedUnsubscription(wsi);
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            printf("Sepa Callback Client received: %s\n", (char *)in);
            break;
        default:
            break;
    }
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
			sepa_session.closing_subscription_code = 0;
			sepa_session.active_subscriptions = 0;
			sepa_session.subscription_list = NULL;
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
	int runningSubscriptions;
	runningSubscriptions = getActiveSubscriptions();
	if (!runningSubscriptions) {
		pthread_mutex_destroy(&(sepa_session.subscription_mutex));
		return EXIT_SUCCESS;
	}
	fprintf(stderr,"Error: %d subscriptions still running!",runningSubscriptions);
	return EXIT_FAILURE;
}

int sepa_subscription_builder(char * sparql_subscription,char * server_address,pSEPA_subscription_params subscription) {
	const char *p;
	const char *sub_address;
	const char *sub_protocol;
	char *sa_ghostcopy;
	int port;
	
	sa_ghostcopy = strdup(server_address);
	if ((sparql_subscription==NULL) || (server_address==NULL) || (sa_ghostcopy==NULL)) {
		fprintf(stderr,"Nullpointer exception in subscription_builder\n");
		return EXIT_FAILURE;
	}
	subscription->subscription_sparql = sparql_subscription;
	if (lws_parse_uri(sa_ghostcopy,&sub_protocol,&sub_address,&port,&p)) {
		fprintf(stderr,"Error while parsing address %s\n",server_address);
		free(sa_ghostcopy);
		return EXIT_FAILURE;
	}
	strcpy(subscription->protocol,sub_protocol);
	strcpy(subscription->address,sub_address);
	free(sa_ghostcopy);
	
	subscription->port = port;
	(subscription->path)[0]='/';
	strncpy(subscription->path+1,p,sizeof(subscription->path)-2);
	(subscription->path)[sizeof(subscription->path)-1]='\0';

	subscription->use_ssl = -1;
	if (!strcmp(subscription->protocol,"ws")) subscription->use_ssl=0;
	else {
		if (!strcmp(subscription->protocol,"wss")) subscription->use_ssl=1;
		else {
			fprintf(stderr,"Requested protocol %s error: must be ws or wss.\n",subscription->protocol);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int kpSubscribe(pSEPA_subscription_params params) {
	int result = -1;
	char *p;
	if ((params!=NULL) && (!pthread_mutex_lock(&(sepa_session.subscription_mutex)))) {
		sepa_session.active_subscriptions++;
		sepa_session.subscription_list = (pSEPA_subscription_params) realloc(sepa_session.subscription_list,sepa_session.active_subscriptions*sizeof(SEPA_subscription_params));
		if (sepa_session.subscription_list==NULL) fprintf(stderr,"Realloc error in kpSubscribe.\n");
		else {
			if (sepa_session.active_subscriptions>1) params->subscription_code = (sepa_session.subscription_list[sepa_session.active_subscriptions-2]).subscription_code+1;
			else params->subscription_code = 1;
			sepa_session.subscription_list[sepa_session.active_subscriptions-1]=*params;
			result = params->subscription_code;
		}
		if ((result==-1) || (pthread_create(&(params->subscription_task),NULL,subscription_thread,(void *) params))) {
			fprintf(stderr,"Problem in creating subscription thread! Exiting...\n");
			result = -1;
		}
		pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	}
	else fprintf(stderr,"kpSubscribe error: parameters are null or mutex not lockable.\n");
	return result;
}

int kpUnsubscribe(pSEPA_subscription_params params) {
	int result=EXIT_FAILURE,i=0,code_index=-1;
	
	printf("Starting unsubscribe procedure!\n");
	if ((params!=NULL) && (!pthread_mutex_lock(&(sepa_session.subscription_mutex)))) {
		while ((i<sepa_session.active_subscriptions) && (code_index==-1)) {
			if ((sepa_session.subscription_list[i]).subscription_code==params->subscription_code) {
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
	else {
		fprintf(stderr,"kpUnubscribe error: parameters are null or mutex not lockable.\n");
		return EXIT_FAILURE;
	}
	
	pthread_mutex_lock(&(sepa_session.subscription_mutex));
	sepa_session.closing_subscription_code = params->subscription_code;
	pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	pthread_join(params->subscription_task,NULL);

	pthread_mutex_lock(&(sepa_session.subscription_mutex));
	sepa_session.closing_subscription_code = 0;
	sepa_session.active_subscriptions--;
	result = EXIT_SUCCESS;
	if (!sepa_session.active_subscriptions) {
		free(sepa_session.subscription_list);
		sepa_session.subscription_list = NULL;
	}
	else {		
		for (i=code_index; i<sepa_session.active_subscriptions; i++) {
			sepa_session.subscription_list[i]=sepa_session.subscription_list[i+1];
		}
		sepa_session.subscription_list = (pSEPA_subscription_params) realloc(sepa_session.subscription_list,sepa_session.active_subscriptions*sizeof(SEPA_subscription_params));
		if (sepa_session.subscription_list==NULL) {
			fprintf(stderr,"Realloc error in kpUnsubscribe\n");
			result = EXIT_FAILURE;
		}
	}
	pthread_mutex_unlock(&(sepa_session.subscription_mutex));

	printf("Unsubscribe executed!\n");
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

pSEPA_subscription_params getSubscriptionList() {
	pSEPA_subscription_params list = NULL;
	if (!pthread_mutex_lock(&(sepa_session.subscription_mutex))) {
		list = sepa_session.subscription_list;
		pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	}
	return list;
}


static void * subscription_thread(void * parameters) {
	struct lws_context_creation_info lwsContext_info;
	struct lws_context *ws_context;
	struct lws_client_connect_info connect_info;
	pSEPA_subscription_params params = (pSEPA_subscription_params) parameters;
	int force_exit=0;
	
	printf("Thread per la sottoscrizione %d attivato.\n",params->subscription_code);
	
	if (params==NULL) {
		fprintf(stderr,"Subscription_thread NullPointerExceptions (parameters)\n");
		pthread_exit(&exit_failure);
	}
	
	memset(&lwsContext_info,0,sizeof lwsContext_info);
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
		pthread_exit(&exit_failure);
	}
	
	connect_info.context = ws_context;
	connect_info.address = params->address;
	connect_info.port = params->port;
	connect_info.path = params->path;
	connect_info.protocol = _getSubscriptionProtocolName();
	connect_info.ssl_connection = params->use_ssl;
	connect_info.host = connect_info.address;
	connect_info.origin = connect_info.address;
	connect_info.ietf_version_or_minus_one = -1;
	connect_info.pwsi = &(params->ws_identifier);
	connect_info.userdata = params->subscription_sparql;
	
	params->ws_identifier = lws_client_connect_via_info(&connect_info);
	
	while (!force_exit) {
		lws_service(ws_context, 50);
		
		pthread_mutex_lock(&(sepa_session.subscription_mutex));
		force_exit = (params->subscription_code==sepa_session.closing_subscription_code);
		pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	}
	lws_context_destroy(ws_context);
	
	printf("Thread per la sottoscrizione %d terminato.\n",params->subscription_code);
	pthread_exit(&exit_success);
}

void fprintfSubscriptionParams(FILE * outstream,SEPA_subscription_params params) {
	if (outstream!=NULL) {
		fprintf(outstream,"SPARQL=%s\nSSL=%d\nPROTOCOL=%s\nADDRESS=%s\nPATH=%s\nPORT=%d\n",
				params.subscription_sparql,
				params.use_ssl,
				params.protocol,
				params.address,
				params.path,
				params.port);
	}
}
