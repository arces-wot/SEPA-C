#ifndef SEPA_UTILITIES
#define SEPA_UTILITIES
#define JSMN_STRICT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsmn.h"

#define COMPLETE_JSON		1
#define INCOMPLETE_JSON		0
#define JSON_ERROR			-4
#define IDENTIFIER_LEN		37
#define _initSepaNode()		{.bindingName=NULL,.type=UNKNOWN,.value=NULL}

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

void fprintfSepaNodes(FILE * outstream,sepaNode * nodeArray,int arraylen);
void freeSepaNodes(sepaNode * nodeArray,int arraylen);
sepaNode buildSepaNode(const char * node_bindingName,FieldType node_type,const char * node_value);
int subscriptionResultsParser(const char * jsonResults,sepaNode * addedNodes,int * addedlen,sepaNode * removedNodes,int * removedlen,notifyProperty * data);
int queryResultsParser(const char * jsonResults,sepaNode * results,int * resultlen);
int checkReceivedJson(char * myjson);

#endif 
