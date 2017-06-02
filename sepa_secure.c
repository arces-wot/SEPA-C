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
	if ((outstream!=NULL) && (client_data.client_id!=NULL) && (client_data.client_secret!=NULL)) {
		fprintf(outstream,"Client id: %s\nClient secret: %s\n\n",client_data.client_id,client_data.client_secret);
	}
 }
 
 sClient registerClient(const char * identity,const char * registrationAddress) {
	CURL *curl;
	CURLcode result;
	struct curl_slist *list = NULL;
	long response_code;
	char *request,*js_buffer;
	HttpJsonResult data;
	sClient resultClientData = _init_sClient();
	jsmn_parser parser;
	jsmntok_t *jstokens;
	int jstok_dim,i,completed=0,parsing_result;
	
	if ((identity==NULL) || (registrationAddress==NULL)) {
		logE("NullPointerException in registerClient.\n");
		return resultClientData;
	}
	
	data.size = 0;
	data.json = (char *) malloc(QUERY_START_BUFFER*sizeof(char));
	if (data.json==NULL) {
		logE("malloc error in registerClient.\n");
		return resultClientData;
	}
	
	result = curl_global_init(CURL_GLOBAL_ALL);
	if (result) {
		logE("curl_global_init() failed.\n");
		return resultClientData;
	}
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, registrationAddress);
		request = (char *) malloc((strlen(identity)+60)*sizeof(char));
		if (request==NULL) logE("malloc error in registerClient.\n");
		else {
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
			}
			else {
				curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
				logI("registerClient Response code is %ld\n",response_code);
				if (response_code==200) {
					jsmn_init(&parser);
					jstok_dim = jsmn_parse(&parser, data.json, data.size, NULL, 0);
					logD("registerClient results=%s - jstok_dim=%d\n",data.json,jstok_dim);
					if (jstok_dim<0) {
						if (jstok_dim==JSMN_ERROR_PART) logD("registerClient Result dimension parsing gave %d\n",jstok_dim);
						else logE("registerClient Result dimension parsing gave %d\n",jstok_dim);
					}
					else {
						jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
						if (jstokens==NULL) {
							logE("registerClient Malloc error in json parsing!\n");
						}
						else {
							jsmn_init(&parser);
							parsing_result = jsmn_parse(&parser, data.json, data.size, jstokens, jstok_dim);
							if (parsing_result<0) {
								logE("registerClient Result dimension parsing gave %d\n",parsing_result);
							}
							else {
								for (i=0; (i<jstok_dim) && (completed<2); i++) {
									if (jstokens[i].type==JSMN_STRING) {
										parsing_result = getJsonItem(data.json,jstokens[i],&js_buffer);
										if (parsing_result==EXIT_SUCCESS) {
											if (!strcmp(js_buffer,"client_id")) {
												logI("registerClient Registration client id: %s",js_buffer);
												resultClientData.client_id = strdup(js_buffer);
												completed++;
											}
											if (!strcmp(js_buffer,"client_secret")) {
												logI("registerClient Registration client secret: %s",js_buffer);
												resultClientData.client_secret = strdup(js_buffer);
												completed++;
											}
										}
										free(js_buffer);
									}
								}
								if ((completed<2) || (resultClientData.client_id==NULL) || (resultClientData.client_secret==NULL)) {
									logE("registerClient Error: client_id/client_secret could not be assessed");
								}
							}
							free(jstokens);
						}
					}
					free(request);
				}
			}
		}
		curl_easy_cleanup(curl);
	}
	else logE("curl_easy_init() failed.\n");
	curl_global_cleanup();
	return resultClientData;
 }
