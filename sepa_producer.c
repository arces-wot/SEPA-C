/*
 * sepa_producer.c
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

#include "sepa_producer.h"


long kpProduce(const char * update_string,const char * http_server,sClient * jwt) {
	CURL *curl;
	CURLcode result;
	struct curl_slist *list = NULL;
	long response_code;
	FILE *nullFile;
	int protocol_used = KPI_PRODUCE_FAIL;
	char *request;
	
	if ((update_string==NULL) || (http_server==NULL)) {
		logE("NullPointerException in kpProduce.\n");
		return KPI_PRODUCE_FAIL;
	}
	
	if (strstr(http_server,"http:")!=NULL) protocol_used = HTTP;
	else {
		if (strstr(http_server,"https:")!=NULL) {
			protocol_used = HTTPS;
			if (jwt==NULL) {
				logE("NullPointerException in JWT with https query request\n");
				return KPI_PRODUCE_FAIL;
			}
		}
		else {
			logE("%s protocol error in kpProduce: only http and https are accepted.\n",http_server);
			return KPI_PRODUCE_FAIL; 
		}
	}
	
	//result = curl_global_init(CURL_GLOBAL_ALL);
	//if (result) {
		//logE("curl_global_init() failed.\n");
		//return KPI_PRODUCE_FAIL;
	//}
	if (http_client_init()==EXIT_FAILURE) return KPI_PRODUCE_FAIL;
	
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, http_server);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, update_string);

		if (protocol_used==HTTPS) {
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // TODO this is not good
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); // TODO this is not good as well
			
			request = (char *) malloc((HTTP_TOKEN_HEADER_SIZE+strlen(jwt->JWT))*sizeof(char));
			if (request==NULL) {
				logE("malloc error in kpQuery. (2)\n");
				curl_easy_cleanup(curl);
				http_client_free();
				return KPI_PRODUCE_FAIL;
			}
			sprintf(request,"Authorization: Bearer %s",jwt->JWT);
			list = curl_slist_append(list, request);
		}
		
		list = curl_slist_append(list, "Content-Type: application/sparql-update");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		
		nullFile = fopen("/dev/null","w");
		if (nullFile==NULL)	{
			logE("Error opening /dev/null.");
			response_code = KPI_PRODUCE_FAIL;
		}
		else {
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullFile);
			result = curl_easy_perform(curl);
			if (result!=CURLE_OK) {
				logE("curl_easy_perform() failed: %s\n",curl_easy_strerror(result));
				response_code = KPI_PRODUCE_FAIL;
			}
			else curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
			fclose(nullFile);
		}
		curl_easy_cleanup(curl);
		if (protocol_used==HTTPS) free(request);
	}
	else {
		logE("curl_easy_init() failed.\n");
		response_code = KPI_PRODUCE_FAIL;
	}
	//curl_global_cleanup();
	http_client_free();
	return response_code;
}
