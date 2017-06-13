/*
 * secure_apis.c
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

/**
 * @brief Example of code used to make a secure registration to SEPA
 * @file secure_apis.c
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 3 jun 2017
 * @example
 */
 
#define SEPA_LOGGER_ERROR
#include <stdio.h>
#include "../sepa_secure.h"
int main(int argc, char **argv) {
	// compile with 
	// $ gcc ./examples/secure_apis.c sepa_utilities.c sepa_consumer.c sepa_secure.c jsmn.c -o secure_apis -lwebsockets -pthread -lcurl `pkg-config --cflags --libs glib-2.0` -L/usr/local/lib
	sClient authorizationData = _init_sClient();
	
	http_client_init();
	if (registerClient("aabb","https://10.143.20.1:8443/oauth/register",&authorizationData)==EXIT_SUCCESS) {
		fprintfSecureClientData(stdout,authorizationData);
		//authorizationData.client_id = strdup("e4f9123f-44c7-49c8-b549-9499c26d9897");
		//authorizationData.client_secret = strdup("91eedc36-c379-431f-bc87-6fcd4e5707fc");
		if (tokenRequest(&authorizationData,"https://10.143.20.1:8443/oauth/token")==EXIT_SUCCESS) {
			fprintfSecureClientData(stdout,authorizationData);
			sClient_free(&authorizationData);
		}
		else printf("Token request was rejected from sepa\n");
	}
	else printf("Registration was rejected from sepa\n");
	http_client_free();
	return 0;
}
