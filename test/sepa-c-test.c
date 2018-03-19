/*
 * sepa-c-test.c
 * 
 * Copyright 2018 Francesco Antoniazzi <francesco@fantoniazzi>
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
 gcc ./sepa-c-test.c ../sepa_utilities.c ../jsmn.c -lcurl `pkg-config --cflags --libs glib-2.0` -o ./sepatest

 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "../sepa_utilities.h"

static void curl_http_test() {
	g_assert_cmpint(http_client_init(),==,EXIT_SUCCESS);
	http_client_free();
}

static void jsmn_parse_test() {
	char json[500],*extracted_item=NULL;
	jsmn_parser parser;
	jsmntok_t *jstokens;
	int jstok_dim,r,nodeLen;
	FILE *query_output;
	sepaNode *nodes;
	
	query_output = fopen("./query_output.json","r");
	g_assert_nonnull(query_output);
	fgets(json,500,query_output);
	json[strlen(json)-1]='\0';
	fclose(query_output);
		
	if (g_test_subprocess()) {
		r = queryResultsParser(json,&nodes,&nodeLen);
		g_assert_cmpint(r,==,EXIT_SUCCESS);
		fprintfSepaNodes(stdout,nodes,nodeLen,"");
		freeSepaNodes(nodes,nodeLen);
		return;
	}
	else {
		jsmn_init(&parser);
		jstok_dim = jsmn_parse(&parser, json, strlen(json), NULL, 0);
		g_assert_cmpint(jstok_dim,==,50);
		jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
		g_assert_nonnull(jstokens);
		jsmn_init(&parser);
		jstok_dim = jsmn_parse(&parser, json, strlen(json), jstokens, jstok_dim);
		g_assert_cmpint(jstok_dim,>=,0);
		g_assert_cmpint(getJsonItem(json,jstokens[0],&extracted_item),==,EXIT_SUCCESS);
		g_assert_cmpstr(extracted_item,==,json);
		free(extracted_item);
		free(jstokens);
	}
	
	g_test_trap_subprocess(NULL,0,0);
	g_test_trap_assert_passed();
	g_test_trap_assert_stdout(" #1 Binding = a; Field type = URI; Value = http://test.it#A;\n #2 Binding = b; Field type = URI; Value = http://test.it#b;\n #3 Binding = c; Field type = LITERAL; Value = mytest;\n #4 Binding = a; Field type = URI; Value = http://test.it#A;\n #5 Binding = b; Field type = URI; Value = http://test.it#b;\n #6 Binding = c; Field type = URI; Value = http://test.it#c;\n");
}

int main(int argc, char **argv) {
	g_test_init(&argc,&argv,NULL);

	// sepa_utilities.c tests
	g_test_add_func("/sepa_utilities/curl_assertions",curl_http_test);
	g_test_add_func("/sepa_utilities/jsmn_assertions",jsmn_parse_test);

	return g_test_run();
}

