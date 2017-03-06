/*
 * producer.c
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

#include "producer.h"


long kpProduce(const char * update_string,const char * http_server) {
	CURL * curl;
	CURLcode result;
	struct curl_slist *list = NULL;
	long response_code;
	FILE * nullFile;
	
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, http_server);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, update_string);
		
		list = curl_slist_append(list, "\"Content-Type: application/sparql-update\"");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		
#ifdef __linux__
		nullFile = fopen("/dev/null","w");
#else
		nullFile = fopen("nul","w");
#endif
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullFile);
		 
		result = curl_easy_perform(curl);
		if (result!=CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(result));
		else {
			curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return response_code;
}
