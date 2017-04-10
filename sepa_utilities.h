#ifndef SEPA_UTILITIES
#define SEPA_UTILITIES
#define JSMN_STRICT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsmn.h"

#define SEPA_LOGGER_INFO

#ifdef SUPER_VERBOSITY
#define SEPA_LOGGER_DEBUG
#endif

#if defined SEPA_LOGGER_ERROR
	#define logE( format , ... ) fprintf (stderr, "ERROR		" format, ##__VA_ARGS__ )
	#define logW( format , ... ) do { if (0) fprintf (stderr, "WARNING		" format, ##__VA_ARGS__ ); } while (0)
	#define logI( format , ... ) do { if (0) fprintf (stderr, "INFO			" format, ##__VA_ARGS__ ); } while (0)
	#define logD( format , ... ) do { if (0) fprintf (stderr, "DEBUG		" format, ##__VA_ARGS__ ); } while (0)
#else
	#ifdef SEPA_LOGGER_WARNING
		#define logE( format , ... ) fprintf (stderr, "ERROR		" format, ##__VA_ARGS__ )
		#define logW( format , ... ) fprintf (stderr, "WARNING		" format, ##__VA_ARGS__ )
		#define logI( format , ... ) do { if (0) fprintf (stderr, "INFO			" format, ##__VA_ARGS__ ); } while (0)
		#define logD( format , ... ) do { if (0) fprintf (stderr, "DEBUG		" format, ##__VA_ARGS__ ); } while (0)
	#else
		#ifdef SEPA_LOGGER_INFO
			#define logE( format , ... ) fprintf (stderr, "ERROR		" format, ##__VA_ARGS__ )
			#define logW( format , ... ) fprintf (stderr, "WARNING		" format, ##__VA_ARGS__ )
			#define logI( format , ... ) fprintf (stderr, "INFO			" format, ##__VA_ARGS__ )
			#define logD( format , ... ) do { if (0) fprintf (stderr, "DEBUG		" format, ##__VA_ARGS__ ); } while (0)
		#else
			#define logE( format , ... ) fprintf (stderr, "ERROR		" format, ##__VA_ARGS__ )
			#define logW( format , ... ) fprintf (stderr, "WARNING		" format, ##__VA_ARGS__ )
			#define logI( format , ... ) fprintf (stderr, "INFO			" format, ##__VA_ARGS__ )
			#define logD( format , ... ) fprintf (stderr, "DEBUG		" format, ##__VA_ARGS__ )
		#endif
	#endif
#endif

#define COMPLETE_JSON			1
#define INCOMPLETE_JSON			0
#define PARSING_ERROR			-100
#define PING_JSON				-101
#define SUBSCRIPTION_ID_JSON	-102
#define NOTIFICATION_JSON		-103
#define UNSUBSCRIBE_CONFIRM		-104
#define IDENTIFIER_ARRAY_LEN	38
#define IDENTIFIER_LAST_INDEX	37
#define IDENTIFIER_STRING_LEN	"%36s"
#define _initSepaNode()			{.bindingName=NULL,.type=UNKNOWN,.value=NULL}

#define BINDING_LEN			6
#define BINDING_NAME		2
#define BINDING_TYPE		5
#define BINDING_VALUE		7


typedef enum field_type {
	URI,
	LITERAL,
	BNODE,
	UNKNOWN
} FieldType;

typedef struct SEPA_Node {
	char *bindingName;
	FieldType type;
	char *value;
} sepaNode;

typedef struct notification_properties {
	int sequence;
	char identifier[IDENTIFIER_ARRAY_LEN];
	// head:vars?
} notifyProperty;

int getJsonItem(char * json,jsmntok_t token,char ** destination);
void fprintfSepaNodes(FILE * outstream,sepaNode * nodeArray,int arraylen,const char * prefix);
void freeSepaNodes(sepaNode * nodeArray,int arraylen);
sepaNode buildSepaNode(char * node_bindingName,char * node_type,char * node_value);
int subscriptionResultsParser(char * jsonResults,sepaNode ** addedNodes,int * addedlen,sepaNode ** removedNodes,int * removedlen,notifyProperty * data);
int queryResultsParser(char * jsonResults,sepaNode ** results,int * resultlen);
int checkReceivedJson(char * myjson);
sepaNode * getResultBindings(char * json,jsmntok_t * tokens,int * outlen);

#endif 
