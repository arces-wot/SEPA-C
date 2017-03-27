/*
 * producer.h
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

#ifndef SEPA_KPI
#define SEPA_KPI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include <libwebsockets.h>

#define KPI_PRODUCE_FAIL		-1
#define FIRST_SUBSCRIPTION_CODE	100

typedef struct subscription_params {
	char * subscription_string;
	char * ws_server;
	int use_ssl;
} SEPA_subscription_params;

long kpProduce(const char * update_string,const char * http_server);
long kpSubscribe(const char * subscription_string,const char * ws_server);

#endif
