/*
 * sepa_aggregator.h
 * 
 * Copyright 2017 Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

/**
 * @brief Header file defining C aggregator functions for SEPA
 * @file sepa_aggregator.h
 * @author Francesco Antoniazzi <francesco.antoniazzi@unibo.it>
 * @date 12 Apr 2017
 *
 * This file contains the functions useful to build-up a SEPA aggregator. We provide function for querying, subscribing and unsubscribing and updates.
 * Requires libcurl, libwebsockets, jsmn. You should not include it in your code only if you are writing a plain consumer or aggregator. In this case, it
 * is strongly suggested to import the related header file.
 * @see sepa_consumer.h
 * @see sepa_producer.h
 * @see https://curl.haxx.se/libcurl/
 * @see https://libwebsockets.org/
 * @see http://zserge.com/jsmn.html
 */

#ifndef SEPA_AGGREGATOR
#define SEPA_AGGREGATOR

#include "sepa_producer.h"
#include "sepa_consumer.h"

#endif
