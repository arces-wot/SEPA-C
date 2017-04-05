/*
 * sepa_utilities.c
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

#include "sepa_utilities.h"

void fprintfSepaNodes(FILE * outstream,sepaNode * nodeArray,int arraylen) {
	int i;
	if (outstream!=NULL) {
		for (i=0; i<arraylen; i++) {
			if (nodeArray[i].bindingName!=NULL) fprintf(outstream,"Binding = %s; ",nodeArray[i].bindingName);
			else fprintf(outstream,"Binding = None; ");
			switch (nodeArray[i].type) {
				case URI:
					fprintf(outstream,"Field type = URI; ");
					break;
				case LITERAL:
					fprintf(outstream,"Field type = LITERAL; ");
					break;
				case BNODE:
					fprintf(outstream,"Field type = BNODE; ");
					break;
				default:
					fprintf(outstream,"Field type = UNKNOWN; ");
					break;
			}
			if (nodeArray[i].value!=NULL) fprintf(outstream,"Value = %s;\n",nodeArray[i].value);
			else fprintf(outstream,"Value = None;\n");
		}
	}
}

void freeSepaNodes(sepaNode * nodeArray,int arraylen) {
	int i;
	for (i=0; i<arraylen; i++) {
		free(nodeArray[i].bindingName);
		free(nodeArray[i].value);
	}
	free(nodeArray);
}

sepaNode buildSepaNode(const char * node_bindingName,FieldType node_type,const char * node_value) {
	sepaNode result;
	result.bindingName = strdup(node_bindingName);
	result.type = node_type;
	result.value = strdup(node_value);
	return result;
}

int subscriptionResultsParser(const char * jsonResults,sepaNode * addedNodes,int * addedlen,sepaNode * removedNodes,int * removedlen,notifyProperty * data) {
	jsmn_parser parser;
	jsmntok_t *jstokens = NULL;
	int parsing_result,jstok_dim,i;
	char *mybufferkey,*mybufferval;
	
	jsmn_init(&parser);
	jstok_dim = jsmn_parse(&parser, jsonResults, strlen(jsonResults), NULL, 0);
	if (jstok_dim<0) return jstok_dim;
	printf("jstok_dim=%d\n",jstok_dim);
	jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
	if (jstokens==NULL) {
		fprintf(stderr,"Malloc error in json parsing!\n");
		parsing_result = JSON_ERROR;
	}
	else {
		parsing_result = jsmn_parse(&parser, jsonResults, strlen(jsonResults), jstokens, (unsigned int) jstok_dim);
		printf("parsing result = %d\n",parsing_result);
		if (parsing_result==0) {

				printf("type=%d; start=%d; end=%d; size=%d\n",jstokens[0].type,jstokens[0].start,jstokens[0].end,jstokens[0].size);
		}
	}
	free(jstokens);
	return parsing_result;
}

int queryResultsParser(const char * jsonResults,sepaNode * results,int * resultlen) {
	return 0;
}

/*int checkReceivedJson(char * myjson) {
	char *jsoncopy,*index;
	int started_tag=0,opened=0,closed=0;
	
	jsoncopy = strdup(myjson);
	for (index=jsoncopy; *index!='\0'; index++) {
		if (started_tag) {
			if (*index!='"') *index=' ';
			else started_tag = 0;
		}
		else {
			if (*index=='"') started_tag = 1;
		}
	}
	for (index=jsoncopy; *index!='\0'; index++) {
		if (*index=='{') opened++;
		if (*index=='}') closed++;
	}
	free(jsoncopy);
	if (opened==closed) return COMPLETE_JSON;
	else return INCOMPLETE_JSON;
}*/
