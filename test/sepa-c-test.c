/*
 * sepa-c-test.c
 * 
 * Copyright 2018 Francesco Antoniazzi <francesco.antoniazzi1991@gmail.com>
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
 gcc ./sepa-c-test.c ../sepa_utilities.c ../sepa_producer.c ../sepa_consumer.c ../jsmn.c -lcurl -pthread `pkg-config --cflags --libs glib-2.0 libwebsockets` -o ./sepatest

 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "../sepa_producer.h"
#include "../sepa_consumer.h"

#define SAFE_MODE 	1
#define UNSAFE_MODE	0

struct sepa_connection {
	int secure;
	char *address;
};

int substr_occurrence(char *str,char *substr) {
	char *position=str;
	int count=0;
	while (position = strstr(position,substr)) {
		count++;
		position++;
	}
	return count;
}

static void curl_http_test() {
	g_assert_cmpint(http_client_init(),==,EXIT_SUCCESS);
	http_client_free();
}

static void jsmn_parse_test() {
	char data[600],*extracted_item=NULL;
	jsmn_parser parser;
	jsmntok_t *jstokens;
	int jstok_dim,r,nodeLen;
	FILE *query_output;
	sepaNode *nodes;
	
	query_output = fopen("./test/query_output.json","r");
	g_assert_nonnull(query_output);
	fgets(data,500,query_output);
	data[strlen(data)-1]='\0';
	fclose(query_output);

	jsmn_init(&parser);
	jstok_dim = jsmn_parse(&parser, data, strlen(data), NULL, 0);
	g_assert_cmpint(jstok_dim,==,50);
	jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
	g_assert_nonnull(jstokens);
	
	jsmn_init(&parser);
	jstok_dim = jsmn_parse(&parser, data, strlen(data), jstokens, jstok_dim);
	g_assert_cmpint(jstok_dim,>=,0);
	g_assert_cmpint(getJsonItem(data,jstokens[0],&extracted_item),==,EXIT_SUCCESS);
	g_assert_cmpstr(extracted_item,==,data);
	free(extracted_item);
	free(jstokens);
	
	r = queryResultsParser(data,&nodes,&nodeLen);
	g_assert_cmpint(r,==,EXIT_SUCCESS);
	sprintfSepaNodes(data,nodes,nodeLen,"");
	freeSepaNodes(nodes,nodeLen);

	g_assert_cmpint(substr_occurrence(data,"Binding = a; Field type = URI; Value = http://test.it#A;"),==,2);
	g_assert_cmpint(substr_occurrence(data,"Binding = b; Field type = URI; Value = http://test.it#b;"),==,2);
	g_assert_cmpint(substr_occurrence(data,"Binding = c; Field type = LITERAL; Value = mytest;"),==,1);
	g_assert_cmpint(substr_occurrence(data,"Binding = c; Field type = URI; Value = http://test.it#c;"),==,1);
}

static void sepa_producer_test(const void* setting) {
	struct sepa_connection *psetting = (struct sepa_connection *) setting;
	char sepa_complete_address[100]="",*query_result=NULL,sepa_path[20]="";
	long result;
	int port;
	
	if (psetting->secure) { 
		//TODO
		g_test_skip("TODO");
		return;
		// secure update and query case
		g_test_message("Safe update and query test");
		port = 8443;
		strcpy(sepa_path,"secure/");
	}
	else {
		g_test_message("Unsafe update and query test");
		port = 8000;
	}
		
	sprintf(sepa_complete_address,"http://%s:%d/%supdate",psetting->address,port,sepa_path);
	result = sepa_update("DELETE {?a ?b ?c} WHERE {?a ?b ?c}",sepa_complete_address,NULL);
	g_assert(result==HTTP_200_OK);
	
	sprintf(sepa_complete_address,"http://%s:%d/%squery",psetting->address,port,sepa_path);
	query_result = sepa_query("SELECT * WHERE {?a ?b ?c}",sepa_complete_address,NULL);
	g_assert_nonnull(query_result);
	g_assert_cmpstr(query_result,==,"{\"head\":{\"vars\":[\"a\",\"b\",\"c\"]},\"results\":{\"bindings\":[]}}");
}

static void sepa_subscriber_test(const void* setting) {
	struct sepa_connection *psetting = (struct sepa_connection *) setting;
	if (psetting->secure) {
		// TODO
		g_test_skip("TODO");
		return;
		// secure update and query case
		g_test_message("Safe subscription test");
	}
	else {
		g_test_message("Unsafe subscription test");
		// TODO
		g_test_skip("TODO");
		return;
	}
}

int main(int argc, char **argv) {
	struct sepa_connection setting_secure = {.secure=SAFE_MODE};
	struct sepa_connection setting_unsecure = {.secure=UNSAFE_MODE};

	g_test_init(&argc,&argv,NULL);
	
	g_assert_cmpint(argc,==,2);
	g_message("SEPA Address = %s",argv[1]);
	setting_secure.address = argv[1];
	setting_unsecure.address = argv[1];
	
	// sepa_utilities.c tests
	g_test_add_func("/sepa_utilities/curl_assertions",curl_http_test);
	g_test_add_func("/sepa_utilities/jsmn_assertions",jsmn_parse_test);
	
	// sepa producer and query tests
	g_test_add_data_func("/sepa_unsafe/producer_assertions",(void*) &setting_unsecure,sepa_producer_test);
	g_test_add_data_func("/sepa_safe/producer_assertions",(void*) &setting_secure,sepa_producer_test);
	
	// sepa_subscription test
	g_test_add_data_func("/sepa_unsafe/subscription_assertions",(void*) &setting_unsecure,sepa_subscriber_test);
	g_test_add_data_func("/sepa_safe/subscription_assertions",(void*) &setting_secure,sepa_subscriber_test);

	return g_test_run();
}

