/*
 * sepa_secure.h
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

/**
 * @brief Header file defining C secure mechanisms for SEPA
 * @file sepa_secure.h
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 3 Jun 2017
 *
 * This file contains the functions useful to communicate in a secure way with the SEPA. Requires libcurl and sepa_utilities. 
 * @see https://curl.haxx.se/libcurl/
 * @see http://zserge.com/jsmn.html
 * @todo Support for authenticity certificates
 */

#ifndef SEPA_SECURE
#define SEPA_SECURE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "sepa_utilities.h"

#define HTTP_CREATED		201
#define _init_sClient()		{.client_id=NULL,.client_secret=NULL}

typedef struct secure_client {
	char *client_id; /**< Here we store the client id after registration */
	char *client_secret; /**< Here we store the client secret key after registration */
} sClient;

/**
 * @brief Registers the client for secure communication with SEPA
 * @param identity: the client's identity
 * @param registrationAddress: the address to which ask for registration
 * @param clientData: pointer to the data structure sClient in which id and key are being stored
 * @return EXIT_SUCCESS, or EXIT_FAILURE
 */
int registerClient(const char * identity,const char * registrationAddress,sClient * clientData);

/**
 * @brief fprintf for sClient
 * 
 * Prints to stream the content of a secure client
 * @param outstream: FILE* to which write
 * @param client_data: sClient structure to be printed
 */
void fprintfSecureClientData(FILE * outstream,sClient client_data);

#endif
