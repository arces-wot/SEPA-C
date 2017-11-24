/*
 * sepa_consumer.c
 * 
 * Copyright 2017 Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "sepa_consumer.h"

#define G_LOG_DOMAIN "SepaConsumer"
#include <glib.h>

SEPA_subscriber sepa_session;

static int exit_success = EXIT_SUCCESS;
static int exit_failure = EXIT_FAILURE;
static pthread_once_t one_initialization = PTHREAD_ONCE_INIT;
static int CHUNK_MAX_SIZE = _DEFAULT_CHUNK_MAX_SIZE;

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
		RX_BUFFER_SIZE
	},
	{
		NULL,
		NULL,
		0,
		0
	}
};

static pSEPA_subscription_params getRaisedSubscription(struct lws *wsi) {
	int i=0,wsi_index=-1;
	pthread_mutex_lock(&(sepa_session.subscription_mutex));
	while ((i<sepa_session.active_subscriptions) && (wsi_index==-1)) {
		if (wsi==((sepa_session.subscription_list[i])->ws_identifier)) wsi_index=i;
		i++;
	}
	pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	if (wsi_index!=-1) return sepa_session.subscription_list[wsi_index];
	return NULL;
}

static int sepa_subscription_callback(	struct lws *wsi,
										enum lws_callback_reasons reason,
										void *user, 
										void *in, 
										size_t len) {
	char *sparql_buffer,*packet_buffer,*receive_buffer;
	int sub_packet_length=50,addedLen,removedLen,parse_result;
	pSEPA_subscription_params raisedSubscription;
	sepaNode *added,*removed;
	notifyProperty n_properties;
	
	char *chunk_buffer;
	int i,end=0;
	size_t chunk_len;
	GTimer *timer;
	
	raisedSubscription = getRaisedSubscription(wsi);
	if (raisedSubscription!=NULL) {
		switch (reason) {
			case LWS_CALLBACK_CLIENT_ESTABLISHED:
				pthread_mutex_lock(&(sepa_session.subscription_mutex));
				if (raisedSubscription->use_ssl!=WS_NOT_SECURE) sub_packet_length += SUBSCRIPTION_AUTH_TOKEN_LEN;
				if (raisedSubscription->subscription_alias!=NULL) sub_packet_length += strlen(raisedSubscription->subscription_alias);
				sub_packet_length += strlen(raisedSubscription->subscription_sparql);
				
				packet_buffer = (char *) malloc((LWS_PRE+sub_packet_length)*sizeof(char));
				if (packet_buffer!=NULL) {
					timer = g_timer_new();
					sparql_buffer = packet_buffer+LWS_PRE;
					sprintf(sparql_buffer,"{\"subscribe\":\"%s\"",raisedSubscription->subscription_sparql);
					if (raisedSubscription->subscription_alias!=NULL) sprintf(sparql_buffer+strlen(sparql_buffer)+1,",\"alias\":\"%s\"",raisedSubscription->subscription_alias);
					if (raisedSubscription->use_ssl!=WS_NOT_SECURE) sprintf(sparql_buffer,"%s,\"authorization\":\"Bearer %s\"",sparql_buffer,raisedSubscription->subscription_authToken);
					strcat(sparql_buffer,"}");
					printf("packet = %s\n",sparql_buffer);

					if (strlen(sparql_buffer)<=CHUNK_MAX_SIZE) {
						g_message("Sending websocket frame...");
						g_timer_start(timer);
						while ((lws_send_pipe_choked(wsi)) && (g_timer_elapsed(timer,NULL)<LWS_PIPE_CHOKED_TIMEOUT));
						lws_write(wsi,sparql_buffer,strlen(sparql_buffer),LWS_WRITE_TEXT);
					}
					else {
						chunk_buffer = sparql_buffer;
						do {
							chunk_len = strlen(chunk_buffer);
							g_timer_start(timer);
							while ((lws_send_pipe_choked(wsi)) && (g_timer_elapsed(timer,NULL)<LWS_PIPE_CHOKED_TIMEOUT));
							if (chunk_buffer==sparql_buffer) {
								g_message("Writing websocket chunk...");
								i=lws_write(wsi,chunk_buffer,CHUNK_MAX_SIZE,LWS_WRITE_TEXT | LWS_WRITE_NO_FIN);
							}
							else {
								if (chunk_len>CHUNK_MAX_SIZE) {
									g_message("Writing websocket chunk...");
									i=lws_write(wsi,chunk_buffer,CHUNK_MAX_SIZE,LWS_WRITE_CONTINUATION | LWS_WRITE_NO_FIN);
								}
								else {
									g_message("Sending websocket frame...");
									i=lws_write(wsi,chunk_buffer,chunk_len,LWS_WRITE_CONTINUATION);
									end = 1;
								}
							}
							chunk_buffer = &chunk_buffer[i];
						} while (!end);
					}
					free(packet_buffer);
					g_timer_destroy(timer);
				}
				else g_critical("Malloc error in sepa_subscription_callback LWS_CALLBACK_CLIENT_ESTABLISHED");
				pthread_mutex_unlock(&(sepa_session.subscription_mutex));
				g_info("Sepa Callback: Connect with server success.");
				break;
			case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
				g_critical("Sepa Callback: Connect with server error.");
				// maybe we need to unsubscribeAll or terminate application
				break;
			case LWS_CALLBACK_CLOSED:
				g_warning("Sepa Callback: LWS_CALLBACK_CLOSED");
				// maybe we need to unsubscribeAll or terminate application
				break;
			case LWS_CALLBACK_CLIENT_RECEIVE:
				pthread_mutex_lock(&(sepa_session.subscription_mutex));
				receive_buffer = (char *) in;
				raisedSubscription->resultBuffer = (char *) realloc(raisedSubscription->resultBuffer,(strlen(raisedSubscription->resultBuffer)+strlen(receive_buffer)+1)*sizeof(char));
				if (raisedSubscription->resultBuffer!=NULL) {
					strcat(raisedSubscription->resultBuffer,receive_buffer);
					parse_result = subscriptionResultsParser(raisedSubscription->resultBuffer,&added,&addedLen,&removed,&removedLen,&n_properties);
					switch (parse_result) {
						case JSMN_ERROR_INVAL:
							g_warning("Sepa Callback Client received error - Invalid JSON Received [id=%s]",raisedSubscription->identifier);
							strcpy(raisedSubscription->resultBuffer,"");
							break;
						case JSMN_ERROR_PART:
							g_info("Sepa Callback Client received partial JSON [id=%s]...",raisedSubscription->identifier);
							break;
						case PARSING_ERROR:
							g_critical("Sepa Callback Client received parsing error: aborting this read [id=%s]",raisedSubscription->identifier);
							strcpy(raisedSubscription->resultBuffer,"");
							break;
						case PING_JSON:
							g_info("Sepa Callback Client received a ping from SEPA");
							break;
						case SUBSCRIPTION_ID_JSON:
							strcpy(raisedSubscription->identifier,(n_properties.identifier)+SUBSCRIPTION_ID_PREAMBLE_LEN);
							g_message("Sepa Callback Client received subscription confirmation #%s",raisedSubscription->identifier);
							(raisedSubscription->subHandler)(added,addedLen,NULL,0);
							break;
						case NOTIFICATION_JSON:
							g_message("Sepa Callback Client notification packet received [sequence=%d, id=%s]",n_properties.sequence,(n_properties.identifier)+SUBSCRIPTION_ID_PREAMBLE_LEN);
							(raisedSubscription->subHandler)(added,addedLen,removed,removedLen);
							break;
						case UNSUBSCRIBE_CONFIRM:
							g_message("Sepa Callback Client unsubscribe confirm received [id=%s]",raisedSubscription->identifier);
							sepa_session.closing_subscription_code = raisedSubscription->subscription_code;
							break;
						default:
							g_warning("Sepa Callback Client unknown parsing code: %d [id=%s]",parse_result,raisedSubscription->identifier);
							break;
					}
				}
				else g_critical("Realloc error in sepa_subscription_callback LWS_CALLBACK_CLIENT_RECEIVE");
				pthread_mutex_unlock(&(sepa_session.subscription_mutex));
				break;
			default:
				break;
		}
	}
	return 0;
}

void _set_chunk_max_size(int size) {
	CHUNK_MAX_SIZE = size;
	g_warning("CHUNK_MAX_SIZE setted to %d",size);
}

static void __sepa_subscriber_init() {
	sepa_session.closing_subscription_code = 0;
	sepa_session.active_subscriptions = 0;
	sepa_session.subscription_list = NULL;
	pthread_mutex_init(&(sepa_session.subscription_mutex),NULL);
	g_message("Subscription engine initialized!");
}

pSEPA_subscriber sepa_subscriber_init() {
	pthread_once(&one_initialization, __sepa_subscriber_init);
	return &sepa_session;
}

int sepa_subscriber_destroy() {
	int runningSubscriptions;
	runningSubscriptions = getActiveSubscriptions();
	if (!runningSubscriptions) {
		pthread_mutex_destroy(&(sepa_session.subscription_mutex));
		return EXIT_SUCCESS;
	}
	g_warning("%d subscriptions still running!",runningSubscriptions);
	return EXIT_FAILURE;
}

int sepa_subscription_builder(char * sparql_subscription,char * subscription_alias,sClient * auth_token,char * server_address,pSEPA_subscription_params subscription) {
	const char *p;
	const char *sub_address;
	const char *sub_protocol;
	char *sa_ghostcopy;
	int port;
	
	sa_ghostcopy = strdup(server_address);
	if ((sparql_subscription==NULL) || (server_address==NULL) || (sa_ghostcopy==NULL)) {
		g_error("Nullpointer exception in subscription_builder");
		return EXIT_FAILURE;
	}
	subscription->subscription_sparql = sparql_subscription;
	if (lws_parse_uri(sa_ghostcopy,&sub_protocol,&sub_address,&port,&p)) {
		g_critical("Error while parsing address %s",server_address);
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

	subscription->subscription_alias = subscription_alias;

	subscription->use_ssl = INVALID_SECURITY_DEFINITION;
	if (!strcmp(subscription->protocol,"ws")) subscription->use_ssl=WS_NOT_SECURE;
	else {
		if (!strcmp(subscription->protocol,"wss")) {
			// TODO in the future some flags should not be there
			subscription->use_ssl=LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK | LCCSCF_USE_SSL;
			if (auth_token==NULL) {
				g_error("Fatal NullPointerException in auth_token field during secure subscription constuction");
				return EXIT_FAILURE;
			}
			else strcpy(subscription->subscription_authToken,auth_token->JWT);
			g_debug("Secure websocket JWT=%s",subscription->subscription_authToken);
		}
		else {
			g_error("Requested protocol %s error: must be ws or wss.",subscription->protocol);
			return EXIT_FAILURE;
		}
	}
	
	subscription->resultBuffer = strdup("");
	strcpy(subscription->identifier,"");
	if (subscription->resultBuffer==NULL) {
		g_error("strdup resultBuffer failure in sepa_subscription_builder");
		return EXIT_FAILURE;
	}
	
	subscription->subHandler = NULL;
	subscription->unsubHandler = NULL;
	return EXIT_SUCCESS;
}

void sepa_setSubscriptionHandlers(SubscriptionHandler subhandler,UnsubscriptionHandler unsubhandler,pSEPA_subscription_params subscription) {
	if ((subscription==NULL) || (subhandler==NULL)) g_error("NullPointerException in sepa_setSubscriptionHandlers");
	else {
		subscription->subHandler = subhandler;
		subscription->unsubHandler = unsubhandler;
	}
}

int kpSubscribe(pSEPA_subscription_params params) {
	int result = -1;
	char *p;
	if (params!=NULL) {
		if (params->subHandler==NULL) g_error("Subscription handler NullPointerException.");
		else {
			if (!pthread_mutex_lock(&(sepa_session.subscription_mutex))) {
				sepa_session.active_subscriptions++;
				sepa_session.subscription_list = (pSEPA_subscription_params *) realloc(sepa_session.subscription_list,sepa_session.active_subscriptions*sizeof(pSEPA_subscription_params));
				if (sepa_session.subscription_list==NULL) g_error("Realloc error in kpSubscribe.");
				else {
					if (sepa_session.active_subscriptions>1) params->subscription_code = (sepa_session.subscription_list[sepa_session.active_subscriptions-2])->subscription_code+1;
					else params->subscription_code = 1;
					sepa_session.subscription_list[sepa_session.active_subscriptions-1]=params;
					result = params->subscription_code;
				}
				if ((result==-1) || (pthread_create(&(params->subscription_task),NULL,subscription_thread,(void *) params))) {
					g_critical("Problem in creating subscription thread! Aborting subscription...");
					result = -1;
				}
				pthread_mutex_unlock(&(sepa_session.subscription_mutex));
			}
			else g_critical("kpSubscribe error: parameters are null or mutex not lockable.");
		}
	}
	return result;
}

int kpUnsubscribe(pSEPA_subscription_params params) {
	int result=EXIT_FAILURE,i=0,code_index=-1;
	char unsubscribeRequest[LWS_PRE+IDENTIFIER_LAST_INDEX+1024];
	GTimer *timer;
	if (params==NULL) {
		g_critical("kpUnsubscribe error: null params request");
		return EXIT_FAILURE;
	}
	else {
		if (!pthread_mutex_lock(&(sepa_session.subscription_mutex))) {
			g_message("Starting unsubscribe procedure!");
			while ((i<sepa_session.active_subscriptions) && (code_index==-1)) {
				if ((sepa_session.subscription_list[i])->subscription_code==params->subscription_code) {
					code_index = i;
				}
				else i++;
			}
			if (i==sepa_session.active_subscriptions) {
				if (!i) g_critical("No active subscriptions: cannot unsubscribe.");
				else g_critical("Nonexistent subscription code %d",params->subscription_code);
				pthread_mutex_unlock(&(sepa_session.subscription_mutex));
				return EXIT_FAILURE;
			}
			else {
				pthread_mutex_unlock(&(sepa_session.subscription_mutex));
				sprintf(unsubscribeRequest+LWS_PRE,"{\"unsubscribe\":\"sepa://spuid/%s\"",params->identifier);
				if (params->use_ssl!=WS_NOT_SECURE) sprintf(unsubscribeRequest+LWS_PRE,"%s,\"authorization\":\"Bearer %s\"",unsubscribeRequest+LWS_PRE,params->subscription_authToken);
				strcat(unsubscribeRequest+LWS_PRE,"}");
				g_message("Sent unsubscription packet %s",unsubscribeRequest+LWS_PRE);
				timer = g_timer_new();
				while ((lws_send_pipe_choked(params->ws_identifier)) && (g_timer_elapsed(timer,NULL)<LWS_PIPE_CHOKED_TIMEOUT));
				lws_write(params->ws_identifier,unsubscribeRequest+LWS_PRE,strlen(unsubscribeRequest+LWS_PRE),LWS_WRITE_TEXT);
				g_timer_destroy(timer);
			}
		}
		else {
			g_error("kpUnubscribe error: mutex not lockable.");
			return EXIT_FAILURE;
		}
	}
	
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
		sepa_session.subscription_list = (pSEPA_subscription_params *) realloc(sepa_session.subscription_list,sepa_session.active_subscriptions*sizeof(pSEPA_subscription_params));
		if (sepa_session.subscription_list==NULL) {
			g_critical("Realloc error in kpUnsubscribe");
			result = EXIT_FAILURE;
		}
	}
	
	if (params->unsubHandler!=NULL) (params->unsubHandler)();
	g_message("Unsubscribe executed!");
	pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	free(params->resultBuffer);
	return result;
}

int getActiveSubscriptions() {
	int result = -1;
	if (!pthread_mutex_lock(&(sepa_session.subscription_mutex))) {
		result = sepa_session.active_subscriptions;
		pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	}
	return result;
}

pSEPA_subscription_params * getSubscriptionList() {
	pSEPA_subscription_params *list = NULL;
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
	
	g_info("Thread for subscription %d launched.",params->subscription_code);
	
	if (params==NULL) {
		g_critical("Subscription_thread parameters NullPointerExceptions");
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
	// TODO in the future some flags should not be there
	lwsContext_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED;
	ws_context = lws_create_context(&lwsContext_info);
	if (ws_context==NULL) {
		g_critical("Creating sepa subscription libwebsocket context failed");
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

	params->ws_identifier = lws_client_connect_via_info(&connect_info);
	g_debug("Subscription %d wsi=%p",params->subscription_code,params->ws_identifier);
	
	while (!force_exit) {
		lws_service(ws_context, 0);
		
		pthread_mutex_lock(&(sepa_session.subscription_mutex));
		force_exit = (params->subscription_code==sepa_session.closing_subscription_code);
		pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	}
	lws_context_destroy(ws_context);
	
	g_info("Thread for subscription %d successfully ended.",params->subscription_code);
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

char * kpQuery(const char * sparql_query,const char * http_server,sClient * jwt) {
	CURL *curl;
	CURLcode result;
	struct curl_slist *list = NULL;
	long response_code;
	int protocol_used = KPI_QUERY_FAIL;
	HttpJsonResult data;
	char * request;
	
	if ((sparql_query==NULL) || (http_server==NULL)) {
		g_critical("NullPointerException in kpProduce.");
		return NULL;
	}
	
	if (strstr(http_server,"http:")!=NULL) protocol_used = HTTP;
	else {
		if (strstr(http_server,"https:")!=NULL) {
			protocol_used = HTTPS;
			if (jwt==NULL) {
				g_critical("NullPointerException in JWT with https query request");
				return NULL;
			}
		}
		else {
			g_critical("%s protocol error in kpProduce: only http and https are accepted.",http_server);
			return NULL; 
		}
	}
	
	data.size = 0;
	data.json = (char *) malloc(QUERY_START_BUFFER*sizeof(char));
	if (data.json==NULL) {
		g_critical("malloc error in kpQuery (1).");
		return NULL;
	}

	if (http_client_init()==EXIT_FAILURE) return NULL;
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, http_server);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sparql_query);
		if (protocol_used==HTTPS) {
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // TODO this is not good
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); // TODO this is not good as well
			
			request = (char *) malloc((HTTP_TOKEN_HEADER_SIZE+strlen(jwt->JWT))*sizeof(char));
			if (request==NULL) {
				g_critical("malloc error in kpQuery. (2)");
				curl_easy_cleanup(curl);
				http_client_free();
				return NULL;
			}
			sprintf(request,"Authorization: Bearer %s",jwt->JWT);
			list = curl_slist_append(list, request);
		}
		list = curl_slist_append(list, "Content-Type: application/sparql-query");
		list = curl_slist_append(list, "Accept: application/sparql-results+json");
		
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, queryResultAccumulator);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		result = curl_easy_perform(curl);
		if (result!=CURLE_OK) {
			g_critical("curl_easy_perform() failed: %s",curl_easy_strerror(result));
			response_code = KPI_QUERY_FAIL;
		}
		else {
			curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
			g_debug("Response code is %ld",response_code);
			if (response_code!=HTTP_200_OK) response_code = KPI_QUERY_FAIL;
		}
		curl_easy_cleanup(curl);
		if (protocol_used==HTTPS) free(request);
	}
	else {
		g_critical("curl_easy_init() failed.");
		response_code = KPI_QUERY_FAIL;
	}
	http_client_free();
	if (response_code==KPI_QUERY_FAIL) return NULL;
	else return data.json;
}
