/*
 * separegister.c
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
 * @brief Secure registration to SEPA
 * @file separegister.c
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 24 jul 2017
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "../sepa_secure.h"

#define REGISTRATION_PATH	"/oauth/register"
#define TOKEN_PATH		"/oauth/token"

int main(int argc, char **argv) {
	sClient authorizationData = _init_sClient();
	char *id,*R_address,*T_address;
	char r_path[] = REGISTRATION_PATH;
	char t_path[] = TOKEN_PATH;
	
	if (argc!=3) {
		fprintf(stderr,"USAGE:\nseparegister [ID] [ADDRESS]\n");
		return EXIT_FAILURE;
	}
	
	id = strdup(argv[1]);
	R_address = (char *) malloc((strlen(argv[2])+strlen(r_path)+1)*sizeof(char));
	T_address = (char *) malloc((strlen(argv[2])+strlen(t_path)+1)*sizeof(char));
	if ((id==NULL) || (R_address==NULL) || (T_address==NULL)) {
		g_error("Malloc error!\n");
		return EXIT_FAILURE;
	}
	strcpy(R_address,argv[2]);
	strcpy(T_address,argv[2]);
	if (argv[2][strlen(argv[2])-1]=='/') {
		strcat(R_address,r_path+1);
		strcat(T_address,t_path+1);
	}
	else {
		strcat(R_address,r_path);
		strcat(T_address,t_path);
	}
	
	http_client_init();
	if (registerClient(id,R_address,&authorizationData)!=EXIT_SUCCESS) {
		g_error("Registration was rejected from sepa\n");
		return EXIT_FAILURE;
	}
	
	if (tokenRequest(&authorizationData,T_address)!=EXIT_SUCCESS) {
		g_error("Token request was rejected from sepa\n");
		return EXIT_FAILURE;
	}

	fprintfSecureClientData(stdout,authorizationData);
	sClient_free(&authorizationData);
	
	http_client_free();
	free(R_address);
	free(T_address);
	free(id);
	return EXIT_SUCCESS;
}
