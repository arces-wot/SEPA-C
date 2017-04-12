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
 * @brief Example of code used to create query the SEPA
 * @file sepaquery.c
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 12 Apr 2017
 * @example
 */

#include "sepa_consumer.h"

#define COMMAND_NAME		argv[0]
#define SEPA_ADDRESS		argv[1]
#define SPARQL_QUERY		argv[2]

void printUsage() {
	fprintf(stderr,"USAGE:\nno argument or 'help': prints this.\n./querysepa [<url>] [<query>] performs query.\n");
}

int main(int argc, char **argv) {
	sepaNode *results;
	int resultsLen,r;
	char *json;
	switch (argc) {
		case 1:
			printUsage();
			return EXIT_SUCCESS;
		case 2:
			if (!strcmp("help",argv[1])) {
				printUsage();
				return EXIT_SUCCESS;
			}
			else {
				fprintf(stderr,"Unrecognized parameter %s\n",argv[1]);
				printUsage();
				return EXIT_FAILURE;
			}
		case 3:
			json = kpQuery(SPARQL_QUERY,SEPA_ADDRESS);
			if (json!=NULL) {
				r = queryResultsParser(json,&results,&resultsLen);
				if (r!=EXIT_SUCCESS) return EXIT_FAILURE;
				fprintfSepaNodes(stdout,results,resultsLen,"");
				freeSepaNodes(results,resultsLen);
				free(json);
				return EXIT_SUCCESS;
			}
			else {
				fprintf(stderr,"NullpointerException!\n");
				return EXIT_FAILURE;
			}
		default:
			fprintf(stderr,"%s Wrong parameter list.\n",COMMAND_NAME);
			printUsage();
			return EXIT_FAILURE;
			break;
	}
}

