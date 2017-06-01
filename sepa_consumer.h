/*
 * sepa_consumer.h
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

/**
 * @brief Header file defining C consumer functions for SEPA
 * @file sepa_consumer.h
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 12 Apr 2017
 *
 * This file contains the functions useful to build-up a SEPA consumer. We provide function for querying, subscribing and unsubscribing.
 * Requires libcurl, libwebsockets, jsmn. You should include it in your code only if you are not writing an Aggregator. In this case, it
 * is strongly suggested to import the related header file.
 * @see sepa_aggregator.h
 * @see https://curl.haxx.se/libcurl/
 * @see https://libwebsockets.org/
 * @see http://zserge.com/jsmn.html
 * @todo support for https and wss
 */

#ifndef SEPA_CONSUMER
#define SEPA_CONSUMER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include "/usr/local/include/libwebsockets.h"
#include "sepa_utilities.h"

#define HTTP								0
#define HTTPS								1
#define INVALID_SECURITY_DEFINITION			-1
#define WSS_SECURE							1
#define WS_NOT_SECURE						0
#define QUERY_START_BUFFER					512
#define PATH_ADDRESS_LEN					100
#define PROTOCOL_LEN						10
#define WS_ADDRESS_LEN						100
#define RX_BUFFER_SIZE						100
#define SUBSCRIPTION_TAG					"subscribe="
#define UNSUBSCRIBE_TAG						"unsubscribe="
#define UNSUBSCRIBE_TAG_LEN					12
#define KPI_QUERY_FAIL						-2
#define SUBSCRIPTION_AUTH_TOKEN_LEN			0
#define _getSubscriptionProtocols()			_protocols
#define _getSubscriptionProtocolName()		_protocols[0].name
#define _getSubscriptionCallback()			_protocols[0].sepa_subscription_callback
#define _getSubscriptionDataSize()			_protocols[0].per_session_data_size
#define _getSubscriptionRxBufferSize()		_protocols[0].rx_buffer_size
#define _initSubscription()					{.subscription_alias=NULL,.subscription_authToken="",.use_ssl=-1,.subscription_code=-1,.identifier="",.protocol="",.address="",.path="",.port=-1}

/**
 * @brief Prototype for subscription notification handler
 * 
 * You must declare a function of this type for every subscription. When a notification is thrown over the subscription,
 * the function is called giving you asynchronous access to removed nodes and added nodes.
 * @param param1 is an array of nodes of kind sepaNode, containing the <b>added</b> bindings of the notification
 * @param param2 is the int length of param1
 * @param param3 is an array of nodes of kind sepaNode, containing the <b>removed</b> bindings of the notification
 * @param param4 is the int lenght of param3
 */
typedef void (*SubscriptionHandler)(sepaNode *,int,sepaNode *,int);

/**
 * @brief Prototype for <b>un</b>subscription notification handler
 * 
 * You may declare a function of this type for every subscription. When unsubscription is requested,
 * the function is called performing last tasks.
 */
typedef void (*UnsubscriptionHandler)(void);

/**
 * @brief This structure contains the parameters of the subscription.
 * 
 * You should definetly consider this as an opaque structure: appropriate functions are provided
 * to setup the subscription.
 * @see sepa_subscriber_init()
 * @see sepa_subscription_builder()
 * @see sepa_setSubscriptionHandlers()
 * @see libwebsockets documentation
 */ 
typedef struct subscription_params {
	char *subscription_sparql; /**< Char array containing the SPARQL of the subscription */
	char subscription_authToken[SUBSCRIPTION_AUTH_TOKEN_LEN];
	char *subscription_alias;
	int use_ssl; /**< 0, if connection is not secure; 1 otherwise */
	int subscription_code; /**< Reserved internal identifier of the subscription. Do <b>not</b> modify. */
	char identifier[IDENTIFIER_ARRAY_LEN]; /**< SEPA-given uuid identifier of the subscription. */
	pthread_t subscription_task; /**< Notification-waiting task. */
	char protocol[PROTOCOL_LEN]; /**< "ws" or "wss" */
	char address[WS_ADDRESS_LEN]; /**< SEPA server address */
	char path[PATH_ADDRESS_LEN]; /**< usually is //sparql */
	int port; /**< Port for SEPA subscription engine */
	struct lws *ws_identifier; /**< WebSocket instance. */
	SubscriptionHandler subHandler; /**< Pointer to notification-handler function */
	UnsubscriptionHandler unsubHandler; /**< Pointer to unsubscription handler function */
	char *resultBuffer; /**< Buffer for data reception */
} SEPA_subscription_params,*pSEPA_subscription_params;

/**
 * @brief Subscription client engine parameters.
 * 
 * This structure is unique in your multi-subscription project. You should
 * consider it as opaque, and access it through the provided mutex, as it is shared between
 * the subscription tasks.
 */
typedef struct sepa_subscriber {
	int active_subscriptions; /**< Number of active subscriptions */
	pSEPA_subscription_params *subscription_list; /**< List of subscriptions and their parameters */
	pthread_mutex_t subscription_mutex; /**< General mutex used to modify this structure. */
	int closing_subscription_code; /**< Reserved. Do <b>not</b> use. */
} SEPA_subscriber,*pSEPA_subscriber;

/**
 * @brief Structure useful for receiving json data with libcurl.
 */
typedef struct query_json {
	size_t size; /**< Json number of characters */
	char *json; /**< Json received */
} QueryJson;

/**
 * @brief Initialization function for the subscription client engine.
 * 
 * Usually this is the first function called when you want to create subscriptions.
 * It initializes internal structures, like the shared mutex.
 * @return EXIT_SUCCESS or EXIT_FAILURE if already initialized or other problems.
 */
int sepa_subscriber_init();

/**
 * @brief Frees memory when subscriptions are not needed anymore.
 * @return EXIT_SUCCESS or EXIT_FAILURE if subscriptions are still running.
 */
int sepa_subscriber_destroy();

/**
 * @brief Retrieves currently running subscriptions and their parameters.
 * @return The array of subscriptions, or NULL
 * @see getActiveSubscriptions()
 */
pSEPA_subscription_params * getSubscriptionList();

/**
 * @return The number of currently running subscriptions, or -1
 */
int getActiveSubscriptions();

/**
 * @brief Prints to outputstream the parameters of a subscription.
 * @param outputstream: the FILE* to which write the information
 * @param params: the subscription object
 */
void fprintfSubscriptionParams(FILE * outputstream,SEPA_subscription_params params);

/**
 * @brief Set-up notification and unsubscription handlers for a subscription
 * @param subhandler: notification pointer-to-function; must be non null
 * @param unsubhandler: unsubscribe pointer-to-function
 * @param subscription: the subscription object
 */
void sepa_setSubscriptionHandlers(SubscriptionHandler subhandler,UnsubscriptionHandler unsubhandler,pSEPA_subscription_params subscription);

/**
 * @brief Set-up parameters for the subscription
 * @param sparql_subscription: string containing the sparql; must be non null
 * @param server_address: for example "ws://sepa.org:1234/sparql"; must be non null
 * @param subscription: pointer to the subscription object; must be non null
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int sepa_subscription_builder(char * sparql_subscription,char * subscription_alias,char * auth_token,char * server_address,pSEPA_subscription_params subscription);

/**
 * @brief Starts the subscription task.
 * @param params: pointer to the subscription object; must be non null
 * @return the subscription code, or -1
 */
int kpSubscribe(pSEPA_subscription_params params);

/**
 * @brief Performs unsubscription.
 * @param params: pointer to the subscription object; must be non null
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int kpUnsubscribe(pSEPA_subscription_params params);

/**
 * @brief Queries the SEPA.
 * 
 * After calling this, you usually call a json parser.
 * @param sparql_query: the sparql containing the query to be sent to SEPA
 * @param http_server: the address of the SEPA engine, for example "http://sepa.org:1234/sparql"
 * @return a string containing the json response or NULL
 * @see queryResultsParser
 */
char * kpQuery(const char * sparql_query,const char * http_server);

#endif // SEPA_CONSUMER
