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

void mySubscriptionNotification(sepaNode * added,int * addedlen,sepaNode * removed,int * removedlen);
void myUnsubscriptionNotification();

int main(int argc, char **argv) {
	SEPA_subscription_params this_subscription = _initSubscription();
	int a;
	sepa_subscriber_init();
	sepa_subscription_builder("SELECT ?a ?b ?c WHERE {<http://francesco> ?a <http://pingpong>. <http://pingpong> ?b ?c}","ws://mml.arces.unibo.it:9000/sparql",&this_subscription);
	sepa_setSubscriptionHandlers(mySubscriptionNotification,myUnsubscriptionNotification,&this_subscription);
	fprintfSubscriptionParams(stdout,this_subscription);
	printf("Exit code = %d\n",kpSubscribe(&this_subscription));
	printf("insert a number to continue: "); scanf("%d",&a);
	
	kpUnsubscribe(&this_subscription);
	
	sepa_subscriber_destroy();
	return 0;
}

void mySubscriptionNotification(sepaNode * added,int * addedlen,sepaNode * removed,int * removedlen) {
	printf("This is a subscription notification!\n");
}

void myUnsubscriptionNotification() {
	printf("This is an unsubscription notification!\n");
}
