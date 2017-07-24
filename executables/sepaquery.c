/*
 * sepaquery.c
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
 * @brief Makes a query to the SEPA
 * @file sepaquery.c
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 24 jul 2017
 */
 
#include <stdio.h>
#include <stdlib.h>
#include "../sepa_consumer.h"

int main(int argc, char **argv) {
	FILE *inputFile;
	char *query_result=NULL;
	int result,n;
	sClient jwt = _init_sClient();
	
	switch (argc) {
		case 3:
			if ((!ioctl(STDIN_FILENO,I_NREAD,&n)) && (n>0)) {
				result = fscanfSecureClientData(stdin,&jwt);
				query_result = kpQuery(argv[1],argv[2],&jwt);
				sClient_free(&jwt);
			}
			else query_result = kpQuery(argv[1],argv[2],NULL);
			break;
		case 4:
			inputFile = fopen(argv[3],"r");
			if (inputFile==NULL) {
				g_error("Error while opening %s",argv[3]);
				return EXIT_FAILURE;
			}
			result = fscanfSecureClientData(argv[3],&jwt);
			if (result==EXIT_SUCCESS) query_result = kpQuery(argv[1],argv[2],&jwt);
			fclose(inputFile);
			sClient_free(&jwt);
			break;
		default:
			fprintf(stderr,"USAGE:\nUnsecure query:\nsepaquery [SPARQL_QUERY] [ADDRESS]\n\nSecure query:\nsepaquery [SPARQL_QUERY] [ADDRESS] [JWT_FILE]\n");
			return EXIT_FAILURE;
	}
	if (query_result==NULL) {
		g_error("NullPointerException in query result.");
		return EXIT_FAILURE;
	}
	printf("%s\n",query_result);
	free(query_result);
	return EXIT_SUCCESS;
}
