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
#include <glib.h>
#include "../sepa_utilities.h"

static void curl_http_test() {
	g_assert_cmpint(http_client_init(),==,EXIT_SUCCESS);
	http_client_free();
}

int main(int argc, char **argv) {
	g_test_init(&argc,&argv,NULL);

	// sepa_utilities.c tests
	g_test_add_func("/sepa_utilities/assertions",curl_http_test);

	return g_test_run();
}

