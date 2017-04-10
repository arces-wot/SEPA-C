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


#include "sepa_consumer.h"

int main(int argc, char **argv) {
	sepaNode *results;
	int resultsLen;
	char *json;
	json = kpQuery("SELECT ?a ?b ?c WHERE {?a ?b ?c}","http://mml.arces.unibo.it:8000/sparql");
	queryResultsParser(json,&results,&resultsLen);
	fprintfSepaNodes(stdout,results,resultsLen,"");
	
	free(results);
	free(json);
	return 0;
}

