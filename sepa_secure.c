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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "sepa_utilities.h"
#include "sepa_secure.h"

#define G_LOG_DOMAIN "SepaSecure"
#include <glib.h>
 
void sClient_free(sClient * client) {
	free(client->client_id);
	free(client->client_secret);
	free(client->JWT);
	free(client->JWTtype);
}
 
void fprintfSecureClientData(FILE * outstream,sClient client_data) {
	if (outstream!=NULL) {
		fprintf(outstream,"Client id: %s\nClient secret: %s\nJWT: %s\nJWT type: %s\nJWT_expiration: %d\n",client_data.client_id,
					client_data.client_secret,
					client_data.JWT,
					client_data.JWTtype,
					client_data.expires_in);
	}
}

int fscanfSecureClientData(FILE * myFile,sClient * dest) {
	char *inputString=NULL,*pch;
	int i=0,str_size=0,result,id_size,secret_size,jwt_size,type_size,expiration_size,iteration=0;
	sClient temp = _init_sClient();
	
    while (!feof(myFile)) {
        if (i==str_size) {
			str_size += 300;
			inputString = (char *) realloc(inputString,str_size*sizeof(char));
			if (inputString==NULL) {
				g_error("realloc error in fscanfSecureClientData (1)");
				return EXIT_FAILURE;
			}
		}
		inputString[i] = getc(myFile);
        i++;
    }
	g_debug("InputString=%s\n",inputString);
	
	pch = strtok(inputString,"\n");
	while ((pch!=NULL) && (iteration<5)) {
		switch (iteration) {
			case 0:
				dest->client_id = (char *) malloc((strlen(pch)-10)*sizeof(char));
				if (dest->client_id==NULL) {
					free(inputString);
					g_error("malloc error in fscanfSecureClientData (client_id)");
					return EXIT_FAILURE;
				}
				sscanf(pch,"Client id: %s",dest->client_id);
				break;
			case 1:
				dest->client_secret = (char *) malloc((strlen(pch)-14)*sizeof(char));
				if (dest->client_secret==NULL) {
					free(inputString);
					g_error("malloc error in fscanfSecureClientData (client_secret)");
					return EXIT_FAILURE;
				}
				sscanf(pch,"Client secret: %s",dest->client_secret);
				break;
			case 2:
				dest->JWT = (char *) malloc((strlen(pch)-4)*sizeof(char));
				if (dest->JWT==NULL) {
					free(inputString);
					g_error("malloc error in fscanfSecureClientData (JWT)");
					return EXIT_FAILURE;
				}
				sscanf(pch,"JWT: %s",dest->JWT);
				break;
			case 3: 
				dest->JWTtype = (char *) malloc((strlen(pch)-9)*sizeof(char));
				if (dest->JWTtype==NULL) {
					free(inputString);
					g_error("malloc error in fscanfSecureClientData (JWTtype)");
					return EXIT_FAILURE;
				}
				sscanf(pch,"JWT type: %s",dest->JWTtype);
				break;
			case 4:
				sscanf(pch,"JWT_expiration: %d",&(dest->expires_in));
				break;
		}
		iteration++;
		pch = strtok(NULL,"\n");
	}		
	free(inputString);
	return EXIT_SUCCESS;
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
		g_error("NullPointerException in registerClient.");
		return EXIT_FAILURE;
	}
	
	data.size = 0;
	data.json = (char *) malloc(REGISTRATION_BUFFER_LEN*sizeof(char));
	if (data.json==NULL) {
		g_error("malloc error in registerClient.");
		return EXIT_FAILURE;
	}
	
	if (http_client_init()==EXIT_FAILURE) return EXIT_FAILURE;
	
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, registrationAddress);
		request = (char *) malloc((strlen(identity)+60)*sizeof(char));
		if (request==NULL) {
			g_error("malloc error in registerClient.");
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
			g_error("registerClient curl_easy_perform() failed: %s",curl_easy_strerror(result));
			return EXIT_FAILURE;
		}
		curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
		g_debug("registerClient Response code is %ld",response_code);
		curl_easy_cleanup(curl);
		http_client_free();
		if (response_code!=HTTP_201_CREATED) return EXIT_FAILURE;

		resultJson = strdup(data.json);
		if (resultJson==NULL) {
			g_critical("registerClient strdup error %s",data.json);
			free(resultJson);
			free(data.json);
			free(request);
			return EXIT_FAILURE;
		}
		free(data.json);
		free(request);
		
		jsmn_init(&parser);
		jstok_dim = jsmn_parse(&parser, resultJson, data.size, NULL, 0);
		g_debug("registerClient results=%s - jstok_dim=%d",resultJson,jstok_dim);
		if (jstok_dim<0) {
			g_critical("registerClient Result dimension parsing (1) gave %d",jstok_dim);
			free(resultJson);
			return EXIT_FAILURE;
		}
		jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
		if (jstokens==NULL) {
			g_critical("registerClient Malloc error in json parsing");
			free(resultJson);
			return EXIT_FAILURE;
		}
		
		jsmn_init(&parser);
		parsing_result = jsmn_parse(&parser, resultJson, data.size, jstokens, jstok_dim);
		if (parsing_result<0) {
			g_critical("registerClient Result dimension parsing (2) gave %d",parsing_result);
			free(resultJson);
			free(jstokens);
			return EXIT_FAILURE;
		}
		for (i=0; (i<jstok_dim) && (completed<2); i++) {
			if (jstokens[i].type==JSMN_STRING) {
				parsing_result = getJsonItem(resultJson,jstokens[i],&js_buffer);
				if (parsing_result!=EXIT_SUCCESS) {
					g_critical("registerClient error in getJsonItem (1)");
					free(resultJson);
					free(jstokens);
					return EXIT_FAILURE;
				}
				if ((!strcmp(js_buffer,"client_id")) || (!strcmp(js_buffer,"client_secret"))) {
					completed++;
					parsing_result = getJsonItem(resultJson,jstokens[i+1],&data_buffer);
					if (parsing_result!=EXIT_SUCCESS) {
						g_critical("registerClient error in getJsonItem (2)");
						free(resultJson);
						free(jstokens);
						free(js_buffer);
						return EXIT_FAILURE;
					}
					if (!strcmp(js_buffer,"client_id")) {
						g_debug("registerClient Registration client id: %s",data_buffer);
						clientData->client_id = strdup(data_buffer);
					}
					else {
						g_debug("registerClient Registration client secret: %s",data_buffer);
						clientData->client_secret = strdup(data_buffer);
					}
				}
			}
		}
		free(resultJson);
		free(jstokens);
		free(js_buffer);
		if (completed<2) {
			g_error("registerClient Error: client_id/client_secret could not be assessed");
			return EXIT_FAILURE;
		}
	}
	else {
		g_critical("curl_easy_init() failed.");
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
		g_error("NullPointerException in tokenRequest");
		return EXIT_FAILURE;
	}
	
	ascii_key = (char *) malloc((strlen(client->client_id)+strlen(client->client_secret)+1)*sizeof(char));
	if (ascii_key==NULL) {
		g_error("Malloc error in tokenRequest");
		return EXIT_FAILURE;
	}
	sprintf(ascii_key,"%s:%s",client->client_id,client->client_secret);
	base64_key = g_base64_encode((guchar *) ascii_key,strlen(ascii_key));
	g_debug("base64(%s)=%s",ascii_key,base64_key);
	free(ascii_key);
	
	if (http_client_init()==EXIT_FAILURE) return EXIT_FAILURE;
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, requestAddress);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // TODO this is not good -2
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); // TODO this is not good as well -2
		
		ascii_key = (char *) malloc((strlen(base64_key)+21)*sizeof(char));
		if (ascii_key==NULL) {
			g_critical("Malloc error in tokenRequest (2)");
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
			g_error("malloc error in tokenRequest.");
			return EXIT_FAILURE;
		}
		
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, queryResultAccumulator);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		result = curl_easy_perform(curl);
		if (result!=CURLE_OK) {
			g_critical("registerClient curl_easy_perform() failed: %s",curl_easy_strerror(result));
			free(data.json);
			return EXIT_FAILURE;
		}
		curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
		g_debug("registerClient Response code is %ld",response_code);
		curl_easy_cleanup(curl);
	}
	//curl_global_cleanup();
	http_client_free();
	if (response_code!=HTTP_201_CREATED) return EXIT_FAILURE;
	
	jsmn_init(&parser);
	jstokens = (jsmntok_t *) malloc(TOKEN_JSON_SIZE*sizeof(jsmntok_t));
	if (jstokens==NULL) {
		g_critical("tokenRequest malloc error in json parsing");
		free(data.json);
		http_client_free();
		return EXIT_FAILURE;
	}
	parsing_result = jsmn_parse(&parser, data.json, data.size, jstokens, TOKEN_JSON_SIZE);
	if (parsing_result<0) {
		free(jstokens);
		g_critical("Result dimension parsing gave %d",parsing_result);
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
			g_critical("tokenRequest getJsonItem error!");
			free(data.json);
			free(jstokens);
			return EXIT_FAILURE;
		}
	}
	if (completed!=3) {
		g_critical("tokenRequest received a bad token confirmation json");
		free(data.json);
		free(jstokens);
		return EXIT_FAILURE;
	}
	free(jsonItem);	
	free(data.json);
	free(jstokens);
	return EXIT_SUCCESS;
}
