/*
 * sepa_utilities.c
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

#include "sepa_utilities.h"

// This variable is the 'client counter' for CURL. It's used in http_client_init and in 'http_client_free'.
// Please don't touch it!
static volatile int CURL_INITIALIZATION = 0;

int http_client_init() {
	// TODO this function might need access control when used by very complex clients
	char executed[15]="(Nothing done)";
	CURLcode result;
	if (CURL_INITIALIZATION>=0) {
		if (CURL_INITIALIZATION==0) {
            strcpy(executed,"(curl init)");
			result = curl_global_init(CURL_GLOBAL_ALL);
			if (result) {
				logE("curl_global_init() failed.\n");
				return EXIT_FAILURE;
			}
		}
		CURL_INITIALIZATION++;
		logI("http_client_init() completed successfully: %d active clients %s\n",CURL_INITIALIZATION,executed);
	}
	return EXIT_SUCCESS;
}

void http_client_free() {
	// TODO this function might need access control when used by very complex clients
	char executed[15]="(Nothing done)";
	if (CURL_INITIALIZATION>0) {
		if (CURL_INITIALIZATION==1) {
            strcpy(executed,"(curl ended)");
            curl_global_cleanup();
		}
		CURL_INITIALIZATION--;
		logI("http_client_free() completed successfully: %d active clients %s\n",CURL_INITIALIZATION,executed);
	}
}

int getJsonItem(char * json,jsmntok_t token,char ** destination) {
	if (*destination!=NULL) free(*destination);
	*destination = strndup(json+token.start,token.end-token.start);
	if (*destination==NULL) {
		logE("Error in getJsonItem.\n");
		return PARSING_ERROR;
	}
	return EXIT_SUCCESS;
}

void fprintfSepaNodes(FILE * outstream,sepaNode * nodeArray,int arraylen,const char * prefix) {
	int i;
	if (outstream!=NULL) {
		if ((nodeArray==NULL) || (arraylen==0)) fprintf(outstream,"%s Empty data\n",prefix);
		else {
			for (i=0; i<arraylen; i++) {
				if (nodeArray[i].bindingName!=NULL) fprintf(outstream,"%s #%d Binding = %s; ",prefix,i+1,nodeArray[i].bindingName);
				else fprintf(outstream,"%s #%d Binding = None; ",prefix,i+1);
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
}

void freeSepaNodes(sepaNode * nodeArray,int arraylen) {
	int i;
	if ((nodeArray!=NULL) && (arraylen>0)){
		for (i=0; i<arraylen; i++) {
			free(nodeArray[i].bindingName);
			free(nodeArray[i].value);
		}
		free(nodeArray);
	}
}

sepaNode buildSepaNode(char * node_bindingName,char * node_type,char * node_value) {
	sepaNode result;
	if ((node_bindingName==NULL) || (node_type==NULL) || (node_value==NULL)) logE("NullpointerException in buildSepaNode");
	else {
		result.bindingName = strdup(node_bindingName);
		if (!strcmp(node_type,URI_STRING)) result.type = URI;
		else {
			if (!strcmp(node_type,LITERAL_STRING)) result.type = LITERAL;
			else {
				if (!strcmp(node_type,BNODE_STRING)) result.type = BNODE;
				else {
					logE("Detected strange node type: %s",node_type);
					result.type = UNKNOWN;
				}
			}
		}
		result.value = strdup(node_value);
		logI("Created node: %s %d %s\n",result.bindingName,result.type,result.value);
		if ((result.bindingName==NULL) || (result.type==UNKNOWN) || (result.value==NULL)) logE("strdup error in buildSepaNode.\n");
	}
	return result;
}

int subscriptionResultsParser(char * jsonResults,sepaNode ** addedNodes,int * addedlen,sepaNode ** removedNodes,int * removedlen,notifyProperty * data) {
	jsmn_parser parser;
	jsmntok_t *jstokens;
	int parsing_result,jstok_dim,i,completed=0;
	char *js_buffer=NULL,*checkA,*checkC;

	if (jsonResults==NULL) {
		logE("NullpointerException in subscriptionResultParser.\n");
		return PARSING_ERROR;
	}
	//if (checkReceivedJson(jsonResults)==INCOMPLETE_JSON) return JSMN_ERROR_PART;
	jsmn_init(&parser);
	jstok_dim = jsmn_parse(&parser, jsonResults, strlen(jsonResults), NULL, 0);
	logD("results=%s - jstok_dim=%d\n",jsonResults,jstok_dim);
	if (jstok_dim<0) {
		if (jstok_dim==JSMN_ERROR_PART) logD("Result dimension parsing gave %d\n",jstok_dim);
		else logE("Result dimension parsing gave %d\n",jstok_dim);
		return jstok_dim;
	}

	if (strstr(jsonResults,"{\"ping\":")!=NULL) {
		strcpy(jsonResults,"");
		return PING_JSON;
	}
	else {
		jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
		if (jstokens==NULL) {
			logE("Malloc error in json parsing!\n");
			strcpy(jsonResults,"");
			return PARSING_ERROR;
		}
		jsmn_init(&parser);
		parsing_result = jsmn_parse(&parser, jsonResults, strlen(jsonResults), jstokens, jstok_dim);
		if (parsing_result<0) {
			free(jstokens);
			if (parsing_result==JSMN_ERROR_PART) logD("Result total parsing gave %d\n",parsing_result);
			else logE("Result dimension parsing gave %d\n",parsing_result);
			return parsing_result;
		}

		if (jstok_dim>3) {
			// notification to subscription case
			parsing_result = EXIT_SUCCESS;
			for (i=0; (i<jstok_dim) && (parsing_result==EXIT_SUCCESS) && (completed<4); i++) {
				if (jstokens[i].type==JSMN_STRING) {
					parsing_result = getJsonItem(jsonResults,jstokens[i],&js_buffer);
					if (parsing_result==EXIT_SUCCESS) {
						if (!strcmp(js_buffer,"addedresults")) {
							logI("Building added results...\n");
							*addedNodes = getResultBindings(jsonResults,&(jstokens[i+3]),addedlen);
							completed++;
						}
						else {
							if (!strcmp(js_buffer,"removedresults")) {
								logI("Building removed results...\n");
								*removedNodes = getResultBindings(jsonResults,&(jstokens[i+3]),removedlen);
								completed++;
							}
							else {
								if (!strcmp(js_buffer,"sequence")) {
									parsing_result = getJsonItem(jsonResults,jstokens[i+1],&js_buffer);
									if (parsing_result==EXIT_SUCCESS) sscanf(js_buffer,"%d",&(data->sequence));
									completed++;
								}
								else {
									if (!strcmp(js_buffer,"spuid")) {
										parsing_result = getJsonItem(jsonResults,jstokens[i+1],&js_buffer);
										if (parsing_result==EXIT_SUCCESS) strcpy(data->identifier,js_buffer);
										completed++;
									}
								}
							}
						}
					}
					else logE("Error while looking for strings in json\n");
				}
			}
			free(js_buffer);
			if (parsing_result==EXIT_SUCCESS) parsing_result = NOTIFICATION_JSON;
			else parsing_result = PARSING_ERROR;
		}
		else {
			// (un)subscription confirm case
			checkA = strstr(jsonResults,"{\"subscribed\":\"");
			checkC = strstr(jsonResults,"{\"unsubscribed\":\"");
			if (checkC!=NULL) parsing_result = UNSUBSCRIBE_CONFIRM;
			else {
				if (checkA!=NULL) {
					sscanf(jsonResults,"{\"subscribed\":\"" IDENTIFIER_STRING_LEN "\"}",data->identifier);
					(data->identifier)[IDENTIFIER_LAST_INDEX]='\0';
					parsing_result = SUBSCRIPTION_ID_JSON;
				}
				else {
					logE("Subscription confirmation is corrupted: %s\n",jsonResults);
					parsing_result = PARSING_ERROR;
				}
			}
		}
		free(jstokens);
	}
	strcpy(jsonResults,"");
	return parsing_result;
}

int queryResultsParser(char * jsonResults,sepaNode ** results,int * resultlen) {
	jsmn_parser parser;
	jsmntok_t *jstokens;
	int jstok_dim,completed=0,parsing_result,i;
	char *js_buffer = NULL;

	if (jsonResults==NULL) {
		logE("NullpointerException in queryResultParser.\n");
		return PARSING_ERROR;
	}

	jsmn_init(&parser);
	jstok_dim = jsmn_parse(&parser, jsonResults, strlen(jsonResults), NULL, 0);
	logD("results=%s - jstok_dim=%d\n",jsonResults,jstok_dim);
	if (jstok_dim<0) {
		if (jstok_dim==JSMN_ERROR_PART) logD("Result dimension parsing gave %d\n",jstok_dim);
		else logE("Result dimension parsing gave %d\n",jstok_dim);
		return jstok_dim;
	}

	jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
	if (jstokens==NULL) {
		logE("Malloc error in json parsing!\n");
		strcpy(jsonResults,"");
		return PARSING_ERROR;
	}
	jsmn_init(&parser);
	parsing_result = jsmn_parse(&parser, jsonResults, strlen(jsonResults), jstokens, jstok_dim);
	if (parsing_result<0) {
		free(jstokens);
		logE("Result dimension parsing gave %d\n",parsing_result);
		return parsing_result;
	}

	parsing_result = EXIT_SUCCESS;
	for (i=0; (i<jstok_dim) && (parsing_result==EXIT_SUCCESS) && (!completed); i++) {
		if (jstokens[i].type==JSMN_STRING) {
			parsing_result = getJsonItem(jsonResults,jstokens[i],&js_buffer);
			if ((parsing_result==EXIT_SUCCESS) && (!strcmp(js_buffer,"results"))) {
				logI("Building added results...\n");
				*results = getResultBindings(jsonResults,&(jstokens[i+3]),resultlen);
				completed++;
			}
		}
	}

	if (js_buffer!=NULL) free(js_buffer);
	free(jstokens);
	return parsing_result;
}

sepaNode * getResultBindings(char * json,jsmntok_t * tokens,int * outlen) {
	/*
	 * See the file getResultBindings-explanation. 
	 * This is pure magic
	 */
	int i,j,k=0,res_index=0;
	
	#ifdef SUPER_VERBOSITY
	char *js_buffer = NULL;
	#endif
	
	char *bindingName=NULL,*bindingType=NULL,*bindingValue=NULL;
	sepaNode *result = NULL;

	if (json==NULL) {
		logE("NullpointerException in getResultBindings\n");
		return NULL;
	}
	*outlen = 0;
	logD("%s\n",json);
	if (tokens[0].type==JSMN_ARRAY) {
		*outlen = tokens[0].size*tokens[1].size;
		logI("Binding number is %d\n",*outlen);
		if (*outlen==0) return NULL;
		result = (sepaNode *) malloc((*outlen)*sizeof(sepaNode));
		if (result==NULL) {
			logE("Malloc error in getResultBindings\n");
			return NULL;
		}
		
		#ifdef SUPER_VERBOSITY
		if (getJsonItem(json,tokens[0],&js_buffer)==PARSING_ERROR) return NULL;
		printf("array=%s - size=%d\n",js_buffer,tokens[0].size);
		#endif
		
		// here comes the wizard
		i=0; j=0;
		while (res_index<*outlen) {
			#ifdef SUPER_VERBOSITY
			if (getJsonItem(json,tokens[j+1],&js_buffer)==PARSING_ERROR) return NULL;
			printf("token[%d]=%s - token[%d].size=%d\n",j+1,js_buffer,j+1,tokens[j+1].size);
			#endif
			
			for (; i<j+tokens[j+1].size*BINDING_LEN; i+=BINDING_LEN+k) {
				if (getJsonItem(json,tokens[i+BINDING_NAME],&bindingName)==PARSING_ERROR) return NULL; // i+2
				logD("Binding Name %d=%s - size=%d\n",BINDING_NAME,bindingName,tokens[i+BINDING_NAME].size);
				
				if (getJsonItem(json,tokens[i+BINDING_TYPE-1],&bindingType)==PARSING_ERROR) return NULL;
				if (!strcmp(bindingType,"datatype")) k=2;
				else k=0;
				
				if (getJsonItem(json,tokens[i+BINDING_TYPE+k],&bindingType)==PARSING_ERROR) return NULL; // i+5+k
				logD("Binding Type %d=%s - size=%d\n",BINDING_TYPE+k,bindingType,tokens[i+BINDING_TYPE+k].size);
				if (getJsonItem(json,tokens[i+BINDING_VALUE+k],&bindingValue)==PARSING_ERROR) return NULL; // i+7+k
				logD("Binding Value %d=%s - size=%d\n",BINDING_VALUE+k,bindingValue,tokens[i+BINDING_VALUE+k].size);
				
				#ifdef SUPER_VERBOSITY
				printf("name=%s type=%s value=%s\n",bindingName,bindingType,bindingValue);
				#endif
				
				result[res_index] = buildSepaNode(bindingName,bindingType,bindingValue);
				res_index++;
			}
			i++;
			j=i;
		}
		free(bindingName);
		free(bindingType);
		free(bindingValue);
		
		#ifdef SUPER_VERBOSITY
		free(js_buffer);
		#endif
		// here the magic ends
	}
	return result;
}

size_t queryResultAccumulator(void * ptr, size_t size, size_t nmemb, HttpJsonResult * data) {
	size_t index = data->size;
    size_t n = (size * nmemb);
    char* tmp;
    data->size += (size * nmemb);
    tmp = realloc(data->json, data->size + 1); // +1 for '\0'
    if (tmp!=NULL) data->json = tmp;
    else {
        if (data->json!=NULL) free(data->json);
        logE("Failed to allocate memory.\n");
        return 0;
    }

    memcpy((data->json + index), ptr, n);
    data->json[data->size] = '\0';
    return size * nmemb;
}
