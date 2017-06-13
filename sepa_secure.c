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
 
void sClient_free(sClient * client) {
	free(client->client_id);
	free(client->client_secret);
	free(client->JWT);
	free(client->JWTtype);
}
 
 void fprintfSecureClientData(FILE * outstream,sClient client_data) {
	if (outstream!=NULL) {
		fprintf(outstream,"Client id: %s\nClient secret: %s\nJWT=%s\nJWT type=%s\nJWT_expiration=%d\n",client_data.client_id,
					client_data.client_secret,
					client_data.JWT,
					client_data.JWTtype,
					client_data.expires_in);
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
	
	if ((identity==NULL) || (registrationAddress==NULL) || (clientData==NULL)) {
		logE("NullPointerException in registerClient.\n");
		return EXIT_FAILURE;
	}
	
	data.size = 0;
	data.json = (char *) malloc(REGISTRATION_BUFFER_LEN*sizeof(char));
	if (data.json==NULL) {
		logE("malloc error in registerClient.\n");
		return EXIT_FAILURE;
	}
	
	//result = curl_global_init(CURL_GLOBAL_ALL);
	//if (result) {
		//logE("curl_global_init() failed.\n");
		//return EXIT_FAILURE;
	//}
	if (http_client_init()==EXIT_FAILURE) return EXIT_FAILURE;
	
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
		curl_easy_cleanup(curl);
		http_client_free();
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
	}
	else {
		logE("curl_easy_init() failed.\n");
		http_client_free();
	}
	return EXIT_SUCCESS;
 }

int tokenRequest(sClient * client,const char * requestAddress) {
	CURL *curl;
	CURLcode result;
	struct curl_slist *list = NULL;
	long response_code;
	gchar *base64_key;
	char *ascii_key,*jsonItem=NULL;
	HttpJsonResult data;
	jsmn_parser parser;
	jsmntok_t *jstokens;
	int i,completed=0,parsing_result;
	
	if ((client==NULL) || (requestAddress==NULL)) {
		logE("NullPointerException in tokenRequest\n");
		return EXIT_FAILURE;
	}
	
	ascii_key = (char *) malloc((strlen(client->client_id)+strlen(client->client_secret)+1)*sizeof(char));
	if (ascii_key==NULL) {
		logE("Malloc error in tokenRequest\n");
		return EXIT_FAILURE;
	}
	sprintf(ascii_key,"%s:%s",client->client_id,client->client_secret);
	base64_key = g_base64_encode((guchar *) ascii_key,strlen(ascii_key));
	logI("base64(%s)=%s\n",ascii_key,base64_key);
	free(ascii_key);
	
	//result = curl_global_init(CURL_GLOBAL_ALL); // TODO ce ne dovrebbe essere uno solo per processo!
	//if (result) {
		//logE("curl_global_init() failed.\n");
		//g_free(base64_key);
		//return EXIT_FAILURE;
	//}
	if (http_client_init()==EXIT_FAILURE) return EXIT_FAILURE;
	
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, requestAddress);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // TODO this is not good -2
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); // TODO this is not good as well -2
		
		ascii_key = (char *) malloc((strlen(base64_key)+21)*sizeof(char));
		if (ascii_key==NULL) {
			logE("Malloc error in tokenRequest (2)\n");
			g_free(base64_key);
			return EXIT_FAILURE;
		}
		sprintf(ascii_key,"Authorization: Basic %s",base64_key);
		list = curl_slist_append(list, "Content-Type: application/json");
		list = curl_slist_append(list, "Accept: application/json");
		list = curl_slist_append(list, ascii_key);
		free(ascii_key);
		g_free(base64_key);
		
		data.size = 0;
		data.json = (char *) malloc(REGISTRATION_BUFFER_LEN*sizeof(char));
		if (data.json==NULL) {
			logE("malloc error in tokenRequest.\n");
			return EXIT_FAILURE;
		}
		
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, queryResultAccumulator);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		result = curl_easy_perform(curl);
		if (result!=CURLE_OK) {
			logE("registerClient curl_easy_perform() failed: %s\n",curl_easy_strerror(result));
			free(data.json);
			return EXIT_FAILURE;
		}
		curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
		logI("registerClient Response code is %ld\n",response_code);
		curl_easy_cleanup(curl);
	}
	//curl_global_cleanup();
	http_client_free();
	if (response_code!=HTTP_200_OK) return EXIT_FAILURE;
	
	jsmn_init(&parser);
	jstokens = (jsmntok_t *) malloc(TOKEN_JSON_SIZE*sizeof(jsmntok_t));
	if (jstokens==NULL) {
		logE("tokenRequest malloc error in json parsing\n");
		free(data.json);
		http_client_free();
		return EXIT_FAILURE;
	}
	parsing_result = jsmn_parse(&parser, data.json, data.size, jstokens, TOKEN_JSON_SIZE);
	if (parsing_result<0) {
		free(jstokens);
		logE("Result dimension parsing gave %d\n",parsing_result);
		return EXIT_FAILURE;
	}
	
	for (i=1; i<TOKEN_JSON_SIZE; i++) {
		if (getJsonItem(data.json,jstokens[i],&jsonItem)==EXIT_SUCCESS) {
			if (!strcmp(jsonItem,"access_token")) {
				getJsonItem(data.json,jstokens[i+1],&(client->JWT));
				completed++;
			}
			else {
				if (!strcmp(jsonItem,"token_type")) {
					getJsonItem(data.json,jstokens[i+1],&(client->JWTtype));
					completed++;
				}
				else {
					if (!strcmp(jsonItem,"expires_in")) {
						getJsonItem(data.json,jstokens[i+1],&jsonItem);
						sscanf(jsonItem,"%d",&(client->expires_in));
						completed++;
					}
				}
			}
		}
		else {
			logE("tokenRequest getJsonItem error!\n");
			free(data.json);
			free(jstokens);
			return EXIT_FAILURE;
		}
	}
	if (completed!=3) {
		logE("tokenRequest received a bad token confirmation json\n");
		free(data.json);
		free(jstokens);
		return EXIT_FAILURE;
	}
	free(jsonItem);	
	free(data.json);
	free(jstokens);
	return EXIT_SUCCESS;
}
