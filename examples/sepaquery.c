/*
 * sepaquery.c
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
 * @brief Example of code used to create query the SEPA
 * @file sepaquery.c
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 12 Apr 2017
 * @example
 */

#include "../sepa_consumer.h"

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
	sClient jwt;
	
	jwt.JWT = strdup("eyJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJTRVBBVGVzdCIsImF1ZCI6WyJodHRwczpcL1wvd290LmFyY2VzLnVuaWJvLml0Ojg0NDNcL3NwYXJxbCIsIndzczpcL1wvd290LmFyY2VzLnVuaWJvLml0Ojk0NDNcL3NwYXJxbCJdLCJuYmYiOjE0OTczNTA3MzQsImlzcyI6Imh0dHBzOlwvXC93b3QuYXJjZXMudW5pYm8uaXQ6ODQ0M1wvb2F1dGhcL3Rva2VuIiwiZXhwIjoxNDk3MzUwNzQwLCJpYXQiOjE0OTczNTA3MzUsImp0aSI6IjM4ZWI2ZWFhLTU3NDQtNDFhZC1hZTQ5LTEyNGZiYmFjZGNkYjpiNmNkNDYwYS0xOTA3LTQ3MzMtODYwMi03OTkzOGRiNTU0OTUifQ.Sx6jwsz9_FBgZEwREHpDt8p8dmDmm-KfxtnhBYsp0yzWxHfcZLHNQOmBM6RSXhWd-pIGlvkz2eyu_pRxTW10oAzPTX6OeugYps44vbMAGQyE0H7PMRD-EUuswd_IkScz123kY6h5G7jzXUJtaZrhsYyEYxbQE0JK7Fg33EFNxmU");
	
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
			if (http_client_init()==EXIT_FAILURE) return EXIT_FAILURE;
			json = kpQuery(SPARQL_QUERY,SEPA_ADDRESS,&jwt);
			http_client_free();
			if (json!=NULL) {
				r = queryResultsParser(json,&results,&resultsLen);
				if (r!=EXIT_SUCCESS) {
					logE("queryResultsParser failed\n");
					return EXIT_FAILURE;
				}
				fprintfSepaNodes(stdout,results,resultsLen,"");
				freeSepaNodes(results,resultsLen);
				free(json);
				return EXIT_SUCCESS;
			}
			else {
				fprintf(stderr,"NullpointerException in sepaquery!\n");
				return EXIT_FAILURE;
			}
		default:
			fprintf(stderr,"%s Wrong parameter list.\n",COMMAND_NAME);
			printUsage();
			return EXIT_FAILURE;
			break;
	}
}

