/*
 * subexample.c
 * 
 * Copyright 2017 Francesco Antoniazzi <francesco@ArcesAlienware>
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


#include <stdio.h>
#include "sepa_consumer.h"

int main(int argc, char **argv) {
	SEPA_subscription_params this_subscription;
	
	sepa_subscriber_init();
	sepa_subscription_builder("SELECT ?a ?b ?c WHERE {?a ?b ?c}","ws://mml.arces.unibo.it:9000/sparql",&this_subscription);
	
	printf("sparql=%s\nuse_ssl=%d\nprotocol=%s\naddress=%s\npath=%s\nport=%d\n",this_subscription.subscription_sparql,this_subscription.use_ssl,this_subscription.protocol,this_subscription.address,this_subscription.path,this_subscription.port);
	sepa_subscriber_destroy();
	return 0;
}

