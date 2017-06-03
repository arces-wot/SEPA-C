/*
 * sepa_secure.c
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
 
 #include "sepa_secure.h"
 
 void fprintfSecureClientData(FILE * outstream,sClient client_data) {
	if (outstream!=NULL) {
		fprintf(outstream,"Client id: %s\nClient secret: %s\n\n",client_data.client_id,client_data.client_secret);
	}
 }
 
int registerClient(const char * identity,const char * registrationAddress, sClient * clientData) {
	CURL *curl;
	CURLcode result;
	struct curl_slist *list = NULL;
	long response_code;
	char *request,*js_buffer=NULL,*data_buffer=NULL;
	HttpJsonResult data;
	jsmn_parser parser;
	jsmntok_t *jstokens;
	int jstok_dim,i,completed=0,parsing_result;
	char *resultJson;
	
	if ((identity==NULL) || (registrationAddress==NULL)) {
		logE("NullPointerException in registerClient.\n");
		return EXIT_FAILURE;
	}
	
	data.size = 0;
	data.json = (char *) malloc(QUERY_START_BUFFER*sizeof(char));
	if (data.json==NULL) {
		logE("malloc error in registerClient.\n");
		return EXIT_FAILURE;
	}
	
	result = curl_global_init(CURL_GLOBAL_ALL);
	if (result) {
		logE("curl_global_init() failed.\n");
		return EXIT_FAILURE;
	}
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, registrationAddress);
		request = (char *) malloc((strlen(identity)+60)*sizeof(char));
		if (request==NULL) {
			logE("malloc error in registerClient.\n");
			return EXIT_FAILURE;
		}
		sprintf(request,"{\"client_identity\":\"%s\",\"grant_types\":[\"client_credentials\"]}",identity);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // TODO this is not good
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); // TODO this is not good as well
		
		list = curl_slist_append(list, "Content-Type: application/json");
		list = curl_slist_append(list, "Accept: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, queryResultAccumulator);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		result = curl_easy_perform(curl);
		if (result!=CURLE_OK) {
			logE("registerClient curl_easy_perform() failed: %s\n",curl_easy_strerror(result));
			return EXIT_FAILURE;
		}
		curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
		logI("registerClient Response code is %ld\n",response_code);
		if (response_code!=HTTP_CREATED) return EXIT_FAILURE;

		resultJson = strdup(data.json);
		if (resultJson==NULL) {
			logE("registerClient strdup error %s\n",data.json);
			free(resultJson);
			free(data.json);
			free(request);
			return EXIT_FAILURE;
		}
		free(data.json);
		free(request);
		
		jsmn_init(&parser);
		jstok_dim = jsmn_parse(&parser, resultJson, data.size, NULL, 0);
		logD("registerClient results=%s - jstok_dim=%d\n",resultJson,jstok_dim);
		if (jstok_dim<0) {
			logE("registerClient Result dimension parsing (1) gave %d\n",jstok_dim);
			free(resultJson);
			return EXIT_FAILURE;
		}
		jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
		if (jstokens==NULL) {
			logE("registerClient Malloc error in json parsing\n");
			free(resultJson);
			return EXIT_FAILURE;
		}
		
		jsmn_init(&parser);
		parsing_result = jsmn_parse(&parser, resultJson, data.size, jstokens, jstok_dim);
		if (parsing_result<0) {
			logE("registerClient Result dimension parsing (2) gave %d\n",parsing_result);
			free(resultJson);
			free(jstokens);
			return EXIT_FAILURE;
		}
		for (i=0; (i<jstok_dim) && (completed<2); i++) {
			if (jstokens[i].type==JSMN_STRING) {
				parsing_result = getJsonItem(resultJson,jstokens[i],&js_buffer);
				if (parsing_result!=EXIT_SUCCESS) {
					logE("registerClient error in getJsonItem (1)");
					free(resultJson);
					free(jstokens);
					return EXIT_FAILURE;
				}
				if ((!strcmp(js_buffer,"client_id")) || (!strcmp(js_buffer,"client_secret"))) {
					completed++;
					parsing_result = getJsonItem(resultJson,jstokens[i+1],&data_buffer);
					if (parsing_result!=EXIT_SUCCESS) {
						logE("registerClient error in getJsonItem (2)");
						free(resultJson);
						free(jstokens);
						free(js_buffer);
						return EXIT_FAILURE;
					}
					if (!strcmp(js_buffer,"client_id")) {
						logD("registerClient Registration client id: %s\n",data_buffer);
						clientData->client_id = strdup(data_buffer);
					}
					else {
						logD("registerClient Registration client secret: %s\n",data_buffer);
						clientData->client_secret = strdup(data_buffer);
					}
				}
			}
		}
		free(resultJson);
		free(jstokens);
		free(js_buffer);
		if (completed<2) {
			logE("registerClient Error: client_id/client_secret could not be assessed\n");
			return EXIT_FAILURE;
		}
		curl_easy_cleanup(curl);
	}
	else logE("curl_easy_init() failed.\n");
	curl_global_cleanup();
	return EXIT_SUCCESS;
 }
