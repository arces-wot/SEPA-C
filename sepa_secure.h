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
 * @see https://developer.gnome.org/glib/
 * @todo Support for authenticity certificates
 * @todo Verify Json Web Token
 */

#ifndef SEPA_SECURE
#define SEPA_SECURE

#define REGISTRATION_BUFFER_LEN		1024
#define TOKEN_JSON_SIZE				7
#define HTTP_TOKEN_HEADER_SIZE		22
#define _init_sClient()		{.client_id=NULL,.client_secret=NULL,.JWT=NULL,.JWTtype=NULL,.expires_in=0}

/**
 * struct secure_client (aka sClient) is the data structure we use to store sepa registration items for a client.
 * You shouldn't modify its content, though it is possible that you need to read it. In particular, you might need the 
 * JSON Web Token which is stored as a string within the field JWT.
 * @see https://tools.ietf.org/html/rfc7519
 */
typedef struct secure_client {
	char *client_id; /**< Here we store the client id after registration */
	char *client_secret; /**< Here we store the client secret key after registration */
	char *JWT; /**< Here we store the java web token */
	char *JWTtype; /**< Here we store the java web token type*/
	int expires_in; /**< Here we store the expiration time of the java web token */
} sClient;

/**
 * @brief frees memory occupied by a well-built sClient (i.e. after a successful tokenRequest)
 * @param client: the sClient to be freed
 * @see sClient
 */
void sClient_free(sClient * client);

/**
 * @brief fprintf for sClient
 * 
 * Prints to stream the content of a secure client
 * @param outstream: FILE* to which write
 * @param client_data: sClient structure to be printed
 */
void fprintfSecureClientData(FILE * outstream,sClient client_data);

/**
 * @brief Inverse of fprintfSecureClientData. 
 *
 * Reads a secure client JWT in the same format it is printed by fprintfSecureClientData.
 * @param myFile: FILE* origin
 * @param dest: pointer to sClient structure destination
 * @return EXIT_SUCCESS, or EXIT_FAILURE
 */
int fscanfSecureClientData(FILE * myFile,sClient * dest);

/**
 * @brief Registers the client for secure communication with SEPA
 * @param identity: the client's identity
 * @param registrationAddress: the address to which ask for registration
 * @param clientData: pointer to the data structure sClient in which id and key are being stored
 * @return EXIT_SUCCESS, or EXIT_FAILURE
 */
int registerClient(const char * identity,const char * registrationAddress,sClient * clientData);

/**
 * @brief Requests for a JWT given the client_id and the client_secret.
 * @param client: the sClient structure where are stored client_id and client_secret. The JWT will be stored as a result here as well
 * @param requestAddress: the http address to which ask for the JWT
 * @return EXIT_SUCCESS, or EXIT_FAILURE
 */
int tokenRequest(sClient * client,const char * requestAddress);
#endif
