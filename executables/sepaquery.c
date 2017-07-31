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
 * Compile with
 gcc ./executables/sepaquery.c jsmn.c sepa_producer.c sepa_consumer.c sepa_utilities.c sepa_secure.c -lcurl -pthread `pkg-config --cflags --libs glib-2.0 libwebsockets` -o ./executables/sepaquery
 */

/**
 * @brief Makes a query to the SEPA
 * @file sepaquery.c
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 24 jul 2017
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "../sepa_consumer.h"

int main(int argc, char **argv) {
	FILE *inputFile;
	char *query_result=NULL;
	int result,n,r,human=0;
	sClient jwt = _init_sClient();
	sepaNode *qResult;
	
	switch (argc) {
		case 5:
			human=1;
		case 4:
			if ((argc==4) && (!strcmp(argv[3],"--human"))) human=1;
			else {
				// secure query reading from file output of separegister
				inputFile = fopen(argv[3],"r");
				if (inputFile==NULL) {
					g_error("Error while opening %s",argv[3]);
					return EXIT_FAILURE;
				}
				result = fscanfSecureClientData(inputFile,&jwt);
				if (result==EXIT_SUCCESS) query_result = kpQuery(argv[1],argv[2],&jwt);
				fclose(inputFile);
				sClient_free(&jwt);
				break;
			}
		case 3:
			if (!isatty(STDIN_FILENO)) {
				// pipelining case with separegister
				result = fscanfSecureClientData(stdin,&jwt);
				query_result = kpQuery(argv[1],argv[2],&jwt);
				sClient_free(&jwt);
			}
			// otherwise unsecure query
			else query_result = kpQuery(argv[1],argv[2],NULL);
			break;
		default:
			fprintf(stderr,"USAGE:\nUnsecure query:\nsepaquery [SPARQL_QUERY] [ADDRESS] [--human]\n\nSecure query:\nsepaquery [SPARQL_QUERY] [ADDRESS] [JWT_FILE] [--human]\n");
			fprintf(stderr,"\nTrick: ./separegister [ID] [ADDRESS] | ./sepaquery [SPARQL] [QUERY_ADDRESS]\n");
			return EXIT_FAILURE;
	}
	if (query_result==NULL) {
		g_error("NullPointerException in query result.");
		return EXIT_FAILURE;
	}
	if (human) {
		r = queryResultsParser(query_result,&qResult,&n);
		if (r!=EXIT_SUCCESS) {
			g_error("Query result parsing failure.");
			return EXIT_FAILURE;
		}
		fprintfSepaNodes(stdout,qResult,n,"");
	}
	else printf("%s\n",query_result);
	free(query_result);
	return EXIT_SUCCESS;
}
