/*
 * sepasubscribe.c
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
 G_MESSAGES_DEBUG=all /path/to/application
 */

/**
 * @brief Makes an update to the SEPA
 * @file sepaupdate.c
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 26 jul 2017
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <glib.h>
#include "../sepa_consumer.h"

volatile long notification_number = 0;
volatile sig_atomic_t end_process = 0;

void mySubscriptionNotification(sepaNode * added,int addedlen,sepaNode * removed,int removedlen) {
	printf(">>>> SUBSCRIPTION NOTIFICATION #%ld\n",notification_number);
	if (added!=NULL) {
		printf("Added %d items:\n",addedlen);
		fprintfSepaNodes(stdout,added,addedlen,"");
		freeSepaNodes(added,addedlen);
	}
	if (removed!=NULL) {
		printf("Removed %d items:\n",removedlen);
		fprintfSepaNodes(stdout,removed,removedlen,"");
		freeSepaNodes(removed,removedlen);
	}
	printf("<<<<\n");
	notification_number++;
}

void myUnsubscriptionNotification() {
	printf(">>>> UNSUBSCRIPTION NOTIFICATION <<<<\n");
}

void interruptHandler(int signalCode) {
	signal(signalCode,SIG_IGN);
	g_message("Caught Ctrl-C\n");
	end_process = 1;
	signal(signalCode,SIG_DFL);
}

int main(int argc, char **argv) {
	FILE *inputFile;
	SEPA_subscription_params this_subscription = _initSubscription();
	sClient jwt = _init_sClient();
	char *alias=NULL;
	int result,stopchar;
	
	if ((argc>3) && (strstr(argv[argc-1],"--alias=")!=NULL)) {
		alias = (char *) malloc(strlen(argv[argc-1])*sizeof(char));
		if (alias==NULL) {
			g_error("Malloc error!");
			return EXIT_FAILURE;
		}
		sscanf(argv[argc-1],"--alias=%s",alias);
	}
	
	switch (argc) {
		case 3:
			// no alias
			sepa_subscriber_init();
			if (!isatty(STDIN_FILENO)) {
				// pipelining case with separegister
				result = fscanfSecureClientData(stdin,&jwt);
				if (result==EXIT_SUCCESS) sepa_subscription_builder(argv[1],NULL,&jwt,argv[2],&this_subscription);
			}
			// otherwise unsecure subscription
			else sepa_subscription_builder(argv[1],NULL,NULL,argv[2],&this_subscription);
			break;
		case 4:
			sepa_subscriber_init();
			if (alias!=NULL) {
				// case 3 with alias
				if (!isatty(STDIN_FILENO)) {
					// pipelining case with separegister
					result = fscanfSecureClientData(stdin,&jwt);
					if (result==EXIT_SUCCESS) sepa_subscription_builder(argv[1],alias,&jwt,argv[2],&this_subscription);
				}
				// otherwise unsecure subscription
				else sepa_subscription_builder(argv[1],alias,NULL,argv[2],&this_subscription);
			}
			else {
				// case 4: secure query reading from file output of separegister, no alias
				inputFile = fopen(argv[3],"r");
				if (inputFile==NULL) {
					g_error("Error while opening %s",argv[3]);
					return EXIT_FAILURE;
				}
				result = fscanfSecureClientData(inputFile,&jwt);
				if (result==EXIT_SUCCESS) sepa_subscription_builder(argv[1],NULL,&jwt,argv[2],&this_subscription);
				fclose(inputFile);
			}
			break;
		case 5:
			// case 4 with alias
			sepa_subscriber_init();
			if (alias==NULL) {
				g_error("Error while identifying alias %s",argv[4]);
				return EXIT_FAILURE;
			}
			inputFile = fopen(argv[3],"r");
			if (inputFile==NULL) {
				g_error("Error while opening %s",argv[3]);
				return EXIT_FAILURE;
			}
			result = fscanfSecureClientData(inputFile,&jwt);
			if (result==EXIT_SUCCESS) sepa_subscription_builder(argv[1],alias,&jwt,argv[2],&this_subscription);
			fclose(inputFile);
			break;
		default:
			fprintf(stderr,"USAGE:\nUnsecure subscription:\nsepasubscribe [SPARQL_SUBSCRIPTION] [ADDRESS]\n\nSecure subscription:\nsepasubscribe [SPARQL_SUBSCRIPTION] [ADDRESS] [JWT_FILE]\n");
			fprintf(stderr,"\nTrick: ./separegister [ID] [ADDRESS] | ./sepasusbcribe [SPARQL_SUBSCRIPTION] [ADDRESS]\n");
			return EXIT_FAILURE;
	}
	
	fprintf(stderr,"/**********************************************\\\n");
	fprintf(stderr,"* PRESS CTRL-C TO STOP MONITORING SUBSCRIPTION *\n");
	fprintf(stderr,"\\**********************************************/\n");
	
	sepa_setSubscriptionHandlers(mySubscriptionNotification,myUnsubscriptionNotification,&this_subscription);
	kpSubscribe(&this_subscription);
	
	signal(SIGINT,interruptHandler);
	while (!end_process) sleep(3);
	
	kpUnsubscribe(&this_subscription);
	sClient_free(&jwt);
	free(alias);
	sepa_subscriber_destroy();
	return EXIT_SUCCESS;
}


