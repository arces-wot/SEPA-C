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


#include "sepa_consumer.h"

SEPA_subscriber sepa_session;

static int exit_success = EXIT_SUCCESS;
static int exit_failure = EXIT_FAILURE;
static pthread_once_t one_initialization = PTHREAD_ONCE_INIT;

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
	
	enum lws_write_protocol writeFlag;
	char *chunk_buffer;
	int i;
	
	raisedSubscription = getRaisedSubscription(wsi);
	if (raisedSubscription!=NULL) {
		switch (reason) {
			case LWS_CALLBACK_CLIENT_ESTABLISHED:
			
				pthread_mutex_lock(&(sepa_session.subscription_mutex));
				if (raisedSubscription->use_ssl==WSS_SECURE) sub_packet_length += SUBSCRIPTION_AUTH_TOKEN_LEN;
				if (raisedSubscription->subscription_alias!=NULL) sub_packet_length += strlen(raisedSubscription->subscription_alias);
				sub_packet_length += strlen(raisedSubscription->subscription_sparql);
				
				packet_buffer = (char *) malloc((LWS_PRE+sub_packet_length)*sizeof(char));
				if (packet_buffer!=NULL) {
					sparql_buffer = packet_buffer+LWS_PRE;
					sprintf(sparql_buffer,"{\"subscribe\":\"%s\"",raisedSubscription->subscription_sparql);
					if (raisedSubscription->subscription_alias!=NULL) sprintf(sparql_buffer+strlen(sparql_buffer)+1,",\"alias\":\"%s\"",raisedSubscription->subscription_alias);
					if (raisedSubscription->use_ssl==WSS_SECURE) sprintf(sparql_buffer+strlen(sparql_buffer)+1,",\"authorization\":\"%s\"",raisedSubscription->subscription_authToken);
					strcat(sparql_buffer,"}");
					
					if (strlen(sparql_buffer)<=CHUNK_MAX_SIZE) {
						lws_write(wsi,sparql_buffer,strlen(sparql_buffer),LWS_WRITE_TEXT);
					}
					else {
						printf("1\n");
						chunk_buffer = sparql_buffer;
						do {
							printf("2\n");
							if (chunk_buffer==sparql_buffer) writeFlag = LWS_WRITE_TEXT | LWS_WRITE_NO_FIN;
							else {
								if (strlen(chunk_buffer)>CHUNK_MAX_SIZE) {
									writeFlag = LWS_WRITE_CONTINUATION | LWS_WRITE_NO_FIN;
									printf("3a\n");
								}
								else {
									writeFlag = LWS_WRITE_CONTINUATION;
									printf("3b\n");
								}
							}
							if (strlen(chunk_buffer)>CHUNK_MAX_SIZE) {
								
								i=lws_write(wsi,chunk_buffer,CHUNK_MAX_SIZE,writeFlag);
								printf("4a %s %d\n",&chunk_buffer[i],i);
							}
							else {
								
								i=lws_write(wsi,chunk_buffer,strlen(chunk_buffer),writeFlag);
								printf("4b %s %d\n",&chunk_buffer[i],i);
							}
							chunk_buffer = &chunk_buffer[i];
							printf("write was done! %d\n",(int)strlen(chunk_buffer));
						} while (writeFlag!=LWS_WRITE_CONTINUATION);
						printf("We exited while loop!\n");
					}
					//printf("%s\n",sparql_buffer);
					
					free(packet_buffer);
				}
				else logE("Malloc error in sepa_subscription_callback LWS_CALLBACK_CLIENT_ESTABLISHED\n");
				pthread_mutex_unlock(&(sepa_session.subscription_mutex));
				logI("Sepa Callback: Connect with server success.\n");
				break;
			case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
				logE("Sepa Callback: Connect with server error.\n");
				// maybe we need to unsubscribeAll or terminate application
				break;
			case LWS_CALLBACK_CLOSED:
				logI("Sepa Callback: LWS_CALLBACK_CLOSED\n");
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
							logW("Sepa Callback Client received error - Invalid JSON Received [id=%s]\n",raisedSubscription->identifier);
							strcpy(raisedSubscription->resultBuffer,"");
							break;
						case JSMN_ERROR_PART:
							logI("Sepa Callback Client received partial JSON [id=%s]...\n",raisedSubscription->identifier);
							break;
						case PARSING_ERROR:
							logE("Sepa Callback Client received parsing error: aborting this read [id=%s]\n",raisedSubscription->identifier);
							strcpy(raisedSubscription->resultBuffer,"");
							break;
						case PING_JSON:
							logI("Sepa Callback Client received a ping from SEPA\n");
							break;
						case SUBSCRIPTION_ID_JSON:
							strcpy(raisedSubscription->identifier,(n_properties.identifier)+SUBSCRIPTION_ID_PREAMBLE_LEN);
							logI("Sepa Callback Client received subscription confirmation #%s\n",raisedSubscription->identifier);
							break;
						case NOTIFICATION_JSON:
							logI("Sepa Callback Client notification packet received [sequence=%d, id=%s]\n",n_properties.sequence,(n_properties.identifier)+SUBSCRIPTION_ID_PREAMBLE_LEN);
							(raisedSubscription->subHandler)(added,addedLen,removed,removedLen);
							break;
						case UNSUBSCRIBE_CONFIRM:
							logI("Sepa Callback Client unsubscribe confirm received [id=%s]\n",raisedSubscription->identifier);
							sepa_session.closing_subscription_code = raisedSubscription->subscription_code;
							break;
						default:
							logW("Sepa Callback Client unknown parsing code: %d [id=%s]\n",parse_result,raisedSubscription->identifier);
							break;
					}
				}
				else logE("Realloc error in sepa_subscription_callback LWS_CALLBACK_CLIENT_RECEIVE\n");
				pthread_mutex_unlock(&(sepa_session.subscription_mutex));
				break;
			default:
				break;
		}
	}
	return 0;
}

static void __sepa_subscriber_init() {
	sepa_session.closing_subscription_code = 0;
	sepa_session.active_subscriptions = 0;
	sepa_session.subscription_list = NULL;
	pthread_mutex_init(&(sepa_session.subscription_mutex),NULL);
	logI("Subscription engine initialized!\n");
}

void sepa_subscriber_init() {
	pthread_once(&one_initialization, __sepa_subscriber_init);
}

int sepa_subscriber_destroy() {
	int runningSubscriptions;
	runningSubscriptions = getActiveSubscriptions();
	if (!runningSubscriptions) {
		pthread_mutex_destroy(&(sepa_session.subscription_mutex));
		return EXIT_SUCCESS;
	}
	logI("Error: %d subscriptions still running!",runningSubscriptions);
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
		logE("Nullpointer exception in subscription_builder\n");
		return EXIT_FAILURE;
	}
	subscription->subscription_sparql = sparql_subscription;
	if (lws_parse_uri(sa_ghostcopy,&sub_protocol,&sub_address,&port,&p)) {
		logE("Error while parsing address %s\n",server_address);
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
			subscription->use_ssl=WSS_SECURE;
			if (auth_token==NULL) {
				logE("Fatal NullPointerException in auth_token field during secure subscription constuction");
				return EXIT_FAILURE;
			}
			else strcpy(subscription->subscription_authToken,auth_token->JWT);
		}
		else {
			logE("Requested protocol %s error: must be ws or wss.\n",subscription->protocol);
			return EXIT_FAILURE;
		}
	}
	
	subscription->resultBuffer = strdup("");
	strcpy(subscription->identifier,"");
	if (subscription->resultBuffer==NULL) {
		logE("strdup resultBuffer failure in sepa_subscription_builder\n");
		return EXIT_FAILURE;
	}
	
	subscription->subHandler = NULL;
	subscription->unsubHandler = NULL;
	return EXIT_SUCCESS;
}

void sepa_setSubscriptionHandlers(SubscriptionHandler subhandler,UnsubscriptionHandler unsubhandler,pSEPA_subscription_params subscription) {
	if ((subscription==NULL) || (subhandler==NULL)) logE("NullPointerException in sepa_setSubscriptionHandlers\n");
	else {
		subscription->subHandler = subhandler;
		subscription->unsubHandler = unsubhandler;
	}
}

int kpSubscribe(pSEPA_subscription_params params) {
	int result = -1;
	char *p;
	if (params!=NULL) {
		if (params->subHandler==NULL) logE("Subscription handler NullPointerException.\n");
		else {
			if (!pthread_mutex_lock(&(sepa_session.subscription_mutex))) {
				sepa_session.active_subscriptions++;
				sepa_session.subscription_list = (pSEPA_subscription_params *) realloc(sepa_session.subscription_list,sepa_session.active_subscriptions*sizeof(pSEPA_subscription_params));
				if (sepa_session.subscription_list==NULL) logE("Realloc error in kpSubscribe.\n");
				else {
					if (sepa_session.active_subscriptions>1) params->subscription_code = (sepa_session.subscription_list[sepa_session.active_subscriptions-2])->subscription_code+1;
					else params->subscription_code = 1;
					sepa_session.subscription_list[sepa_session.active_subscriptions-1]=params;
					result = params->subscription_code;
				}
				if ((result==-1) || (pthread_create(&(params->subscription_task),NULL,subscription_thread,(void *) params))) {
					logE("Problem in creating subscription thread! Aborting subscription...\n");
					result = -1;
				}
				pthread_mutex_unlock(&(sepa_session.subscription_mutex));
			}
			else logE("kpSubscribe error: parameters are null or mutex not lockable.\n");
		}
	}
	return result;
}

int kpUnsubscribe(pSEPA_subscription_params params) {
	int result=EXIT_FAILURE,i=0,code_index=-1;
	char unsubscribeRequest[LWS_PRE+IDENTIFIER_LAST_INDEX+30];
	if (params==NULL) {
		logE("kpUnsubscribe error: null params request\n");
		return EXIT_FAILURE;
	}
	else {
		if (!pthread_mutex_lock(&(sepa_session.subscription_mutex))) {
			logI("Starting unsubscribe procedure!\n");
			while ((i<sepa_session.active_subscriptions) && (code_index==-1)) {
				if ((sepa_session.subscription_list[i])->subscription_code==params->subscription_code) {
					code_index = i;
				}
				else i++;
			}
			if (i==sepa_session.active_subscriptions) {
				if (!i) logW("No active subscriptions: cannot unsubscribe.\n");
				else logW("Nonexistent subscription code %d\n",params->subscription_code);
				pthread_mutex_unlock(&(sepa_session.subscription_mutex));
				return EXIT_FAILURE;
			}
			else {
				pthread_mutex_unlock(&(sepa_session.subscription_mutex));
				sprintf(unsubscribeRequest+LWS_PRE,"{\"unsubscribe\":\"sepa://subscription/%s\"}",params->identifier);
				logD("%s",unsubscribeRequest+LWS_PRE);
				lws_write(params->ws_identifier,unsubscribeRequest+LWS_PRE,strlen(unsubscribeRequest+LWS_PRE),LWS_WRITE_TEXT);
				logI("Sent unsubscription packet...\n");
			}
		}
		else {
			logE("kpUnubscribe error: mutex not lockable.\n");
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
			logE("Realloc error in kpUnsubscribe\n");
			result = EXIT_FAILURE;
		}
	}
	
	if (params->unsubHandler!=NULL) (params->unsubHandler)();
	logI("Unsubscribe executed!\n");
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
	
	logI("Thread per la sottoscrizione %d attivato.\n",params->subscription_code);
	
	if (params==NULL) {
		logE("Subscription_thread NullPointerExceptions (parameters)\n");
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
		logE("Creating sepa subscription libwebsocket context failed\n");
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
	logD("Subscription %d wsi=%p\n",params->subscription_code,params->ws_identifier);
	
	while (!force_exit) {
		lws_service(ws_context, 50);
		
		pthread_mutex_lock(&(sepa_session.subscription_mutex));
		force_exit = (params->subscription_code==sepa_session.closing_subscription_code);
		pthread_mutex_unlock(&(sepa_session.subscription_mutex));
	}
	lws_context_destroy(ws_context);
	
	logI("Thread per la sottoscrizione %d terminato.\n",params->subscription_code);
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
		logE("NullPointerException in kpProduce.\n");
		return NULL;
	}
	
	if (strstr(http_server,"http:")!=NULL) protocol_used = HTTP;
	else {
		if (strstr(http_server,"https:")!=NULL) {
			protocol_used = HTTPS;
			if (jwt==NULL) {
				logE("NullPointerException in JWT with https query request\n");
				return NULL;
			}
		}
		else {
			logE("%s protocol error in kpProduce: only http and https are accepted.\n",http_server);
			return NULL; 
		}
	}
	
	data.size = 0;
	data.json = (char *) malloc(QUERY_START_BUFFER*sizeof(char));
	if (data.json==NULL) {
		logE("malloc error in kpQuery (1).\n");
		return NULL;
	}
	
	//result = curl_global_init(CURL_GLOBAL_ALL);
	//if (result) {
		//logE("curl_global_init() failed.\n");
		//return NULL;
	//}
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
				logE("malloc error in kpQuery. (2)\n");
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
			logE("curl_easy_perform() failed: %s\n",curl_easy_strerror(result));
			response_code = KPI_QUERY_FAIL;
		}
		else {
			curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
			logI("Response code is %ld\n",response_code);
			if (response_code!=HTTP_200_OK) response_code = KPI_QUERY_FAIL;
		}
		curl_easy_cleanup(curl);
		if (protocol_used==HTTPS) free(request);
	}
	else {
		logE("curl_easy_init() failed.\n");
		response_code = KPI_QUERY_FAIL;
	}
	//curl_global_cleanup();
	http_client_free();
	if (response_code==KPI_QUERY_FAIL) return NULL;
	else return data.json;
}
