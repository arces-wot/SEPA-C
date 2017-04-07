#ifndef SEPA_UTILITIES
#define SEPA_UTILITIES
#define JSMN_STRICT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsmn.h"

#define COMPLETE_JSON			1
#define INCOMPLETE_JSON			0
#define PARSING_ERROR			-100
#define PING_JSON				-101
#define SUBSCRIPTION_ID_JSON	-102
#define NOTIFICATION_JSON		-103
#define IDENTIFIER_LEN			37
#define _initSepaNode()			{.bindingName=NULL,.type=UNKNOWN,.value=NULL}

#define IDLEPOINT			0
#define NOTIFICATION		1
#define SEQUENCE			2
#define OBJECTNAME			3
#define BINDINGS			4

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
	char identifier[IDENTIFIER_LEN];
	// head:vars?
} notifyProperty;

int getJsonItem(char * json,jsmntok_t token,char ** destination);
void fprintfSepaNodes(FILE * outstream,sepaNode * nodeArray,int arraylen);
void freeSepaNodes(sepaNode * nodeArray,int arraylen);
sepaNode buildSepaNode(const char * node_bindingName,FieldType node_type,const char * node_value);
int subscriptionResultsParser(char * jsonResults,sepaNode * addedNodes,int * addedlen,sepaNode * removedNodes,int * removedlen,notifyProperty * data);
int queryResultsParser(const char * jsonResults,sepaNode * results,int * resultlen);
//int checkReceivedJson(char * myjson);
sepaNode * getResultBindings(char * json,jsmntok_t * tokens,int * outlen);

#endif 
