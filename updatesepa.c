/*
 * updateSepa.c
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

#include "sepa_kpi.h"

#define COMMAND_NAME		argv[0]

void printUsage() {
	printf("USAGE:\nno argument or 'help': prints this.\n'prompt' asks for url and update interactively.\n<url> <update> performs update directly\n");
}

int main(int argc, char **argv) {
	long outCode;
	char sepaAddress[100];
	char update[300];
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
			if (!strcmp("prompt",argv[1])) {
				printf("Insert SEPA address: "); scanf("%s%*c",sepaAddress);
				printf("Insert one line update: "); gets(update);
				outCode = kpProduce(update,sepaAddress);
				printf("Http post output code=%ld\n",outCode);
				return (int) outCode;
			}
			else {
				fprintf(stderr,"%s Missing parameter.\n",COMMAND_NAME);
				printUsage();
				return EXIT_FAILURE;
			}
		}
	case 3:
		outCode = kpProduce(argv[2],argv[1]);
		printf("Http post output code=%ld\n",outCode);
		return (int) outCode;
	default:
		fprintf(stderr,"%s Wrong parameter list.\n",COMMAND_NAME);
		printUsage();
		return EXIT_FAILURE;
	}
}
