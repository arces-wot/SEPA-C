/*
 * sepaupdate.c
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
 * @brief Makes an update to the SEPA
 * @file sepaupdate.c
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 26 jul 2017
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "../sepa_producer.h"

int main(int argc, char **argv) {
	FILE *inputFile;
	int result,n;
	long update_result;
	sClient jwt = _init_sClient();
	
	switch (argc) {
		case 3:
			if (!isatty(STDIN_FILENO)) {
				// pipelining case with separegister
				result = fscanfSecureClientData(stdin,&jwt);
				update_result = kpProduce(argv[1],argv[2],&jwt);
				sClient_free(&jwt);
			}
			// otherwise unsecure query
			else update_result = kpProduce(argv[1],argv[2],NULL);
			break;
		case 4:
			// secure query reading from file output of separegister
			inputFile = fopen(argv[3],"r");
			if (inputFile==NULL) {
				g_error("Error while opening %s",argv[3]);
				return EXIT_FAILURE;
			}
			result = fscanfSecureClientData(inputFile,&jwt);
			if (result==EXIT_SUCCESS) update_result = kpProduce(argv[1],argv[2],&jwt);
			fclose(inputFile);
			sClient_free(&jwt);
			break;
		default:
			fprintf(stderr,"USAGE:\nUnsecure update:\nsepaupdate [SPARQL_UPDATE] [ADDRESS]\n\nSecure update:\nsepaupdate [SPARQL_UPDATE] [ADDRESS] [JWT_FILE]\n");
			fprintf(stderr,"\nTrick: ./separegister [ID] [ADDRESS] | ./sepaquery [SPARQL_UPDATE] [UPDATE_ADDRESS]\n");
			return EXIT_FAILURE;
	}
	if (update_result!=HTTP_200_OK) {
		g_critical("Sepa response code: %ld",update_result);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
