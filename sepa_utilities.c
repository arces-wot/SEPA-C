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

int subscriptionResultsParser(const char * jsonResults,sepaNode * addedNodes,int * addedlen,sepaNode * removedNodes,int * removedlen) {
	return 0;
}

int queryResultsParser(const char * jsonResults,sepaNode * results,int * resultlen) {
	return 0;
}

char * kpQuery(const char * sparql_query,const char * sparql_address) {
	return NULL;
}

int checkReceivedJson(char * myjson) {
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
}
