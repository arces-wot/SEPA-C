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

int getJsonItem(char * json,jsmntok_t token,char ** destination) {
	if (*destination!=NULL) free(*destination);
	*destination = strndup(json+token.start,token.end-token.start);
	if (*destination==NULL) {
		fprintf(stderr,"Error in getJsonItem.\n");
		return PARSING_ERROR;
	}
	return EXIT_SUCCESS;
}

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

int subscriptionResultsParser(char * jsonResults,sepaNode * addedNodes,int * addedlen,sepaNode * removedNodes,int * removedlen,notifyProperty * data) {
	jsmn_parser parser;
	jsmntok_t *jstokens;
	int parsing_result,jstok_dim,i,fflag,next=IDLEPOINT,seqflag=0,notflag=0;
	char *js_buffer = NULL;
	
	jstok_dim = jsmn_parse(&parser, jsonResults, strlen(jsonResults), NULL, 0);
	if (jstok_dim<0) return jstok_dim;
	
	if (strstr(jsonResults,"{\"ping\":")!=NULL) {
		strcpy(jsonResults,"");
		return PING_JSON;
	}
	else {
		jsmn_init(&parser);
		jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
		if (jstokens==NULL) {
			fprintf(stderr,"Malloc error in json parsing!\n");
			strcpy(jsonResults,"");
			return PARSING_ERROR;
		}
		parsing_result = jsmn_parse(&parser, jsonResults, strlen(jsonResults), jstokens, jstok_dim);
		if (parsing_result<0) {
			free(jstokens);
			return parsing_result;
		}
		
		if (parsing_result>3) {
			// notification to subscription case
			parsing_result = EXIT_SUCCESS;
			for (i=0; (i<jstok_dim) && (parsing_result==EXIT_SUCCESS); i++) {
				switch (jstokens[i].type) {
					case JSMN_OBJECT:
						parsing_result = getJsonItem(jsonResults,jstokens[i+1],&js_buffer);
						if (jstokens[i+1].type==JSMN_STRING) {
							printf("jsonitem=%s\n",js_buffer);
							if (parsing_result==EXIT_SUCCESS) {
								if (!strcmp(js_buffer,"addedresults")) addedNodes = getResultBindings(jsonResults,&(jstokens[i+4]),addedlen);
								if (!strcmp(js_buffer,"removedresults")) removedNodes = getResultBindings(jsonResults,&(jstokens[i+4]),removedlen);
							}
						}
						break;
					case JSMN_STRING:
						parsing_result = getJsonItem(jsonResults,jstokens[i],&js_buffer);
						if (parsing_result==EXIT_SUCCESS) {
							if (!seqflag) {
								seqflag = !strcmp(js_buffer,"sequence");
								parsing_result = getJsonItem(jsonResults,jstokens[i+1],&js_buffer);
								if ((parsing_result==EXIT_SUCCESS) && (seqflag)) sscanf(js_buffer,"%d",&(data->sequence));
							}
							if (!notflag) {
								notflag = !strcmp(js_buffer,"notification");
								parsing_result = getJsonItem(jsonResults,jstokens[i+1],&js_buffer);
								if ((parsing_result==EXIT_SUCCESS) && (notflag)) strcpy(data->identifier,js_buffer);
							}
						}
						break;
					default:
						break;
				}
			}
			free(js_buffer);
			if (parsing_result==EXIT_SUCCESS) parsing_result = NOTIFICATION_JSON;
		}
		else {
			// subscription confirm case
			parsing_result = getJsonItem(jsonResults,jstokens[2],&js_buffer);
			if (parsing_result==EXIT_SUCCESS) {
				strcpy(data->identifier,js_buffer);
				free(js_buffer);
				parsing_result = SUBSCRIPTION_ID_JSON;
			}
		}
		free(jstokens);
	}
	strcpy(jsonResults,"");		
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
}
*/

sepaNode * getResultBindings(char * json,jsmntok_t * tokens,int * outlen) {
	/*
	 * See the file getResultBindings-explanation
	 */
	int i,j,res_index=0;
	char *js_buffer = NULL;
	sepaNode *result;
	
	//printf("%s\n",json);
	if (tokens[0].type==JSMN_ARRAY) {
		*outlen = tokens[0].size*tokens[1].size;
		result = (sepaNode *) malloc(outlen*sizeof(sepaNode));
		//if (getJsonItem(json,tokens[0],&js_buffer)==PARSING_ERROR) return NULL;
		//printf("array=%s - size=%d\n",js_buffer,tokens[0].size);
		i=0;
		// THAT'S A COMPLEXXX PARSINGGGGG!!!!!
		for (j=0; j<tokens[1].size*BINDING_LEN*tokens[0].size; j=i) {
			//if (getJsonItem(json,tokens[j+1],&js_buffer)==PARSING_ERROR) return NULL;
			//printf("token[%d]=%s - token[%d].size=%d\n",j+1,js_buffer,j+1,tokens[j+1].size);
			for (i; i<j+tokens[j+1].size*BINDING_LEN; i+=BINDING_LEN) {
				if (getJsonItem(json,tokens[i+BINDING_NAME],&js_buffer)==PARSING_ERROR) return NULL;
				//printf("internal token %d=%s - size=%d\n",BINDING_NAME,js_buffer,tokens[i+BINDING_NAME].size);
				result[outlen].binding
				if (getJsonItem(json,tokens[i+BINDING_TYPE],&js_buffer)==PARSING_ERROR) return NULL;
				//printf("internal token %d=%s - size=%d\n",BINDING_TYPE,js_buffer,tokens[i+BINDING_TYPE].size);
				if (getJsonItem(json,tokens[i+BINDING_VALUE],&js_buffer)==PARSING_ERROR) return NULL;
				//printf("internal token %d=%s - size=%d\n",BINDING_VALUE,js_buffer,tokens[i+BINDING_VALUE].size);
				
				res_index++;
			}
			//printf("i=%d\n",i);
			i++;
		}
		free(js_buffer);
	}
	return NULL;
}
