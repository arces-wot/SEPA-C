#include <stdio.h>
#include <stdlib.h>

#include "query.h"
#include "update.h"
#include "subscription.h"

SubscriptionChannel sc, ssc;
Subscription *mySub, *mysSub;
sClient in_jwt;

void how_to_make_an_unsecure_query();
void how_to_make_a_secure_query();
void how_to_make_an_unsecure_update();
void how_to_make_a_secure_update();
void how_to_make_an_unsecure_subscription();
void how_to_make_a_secure_subscription();
void how_to_unsubscribe();

int main(int argc, char **argv) {
    int i;
    how_to_make_an_unsecure_query();
    how_to_make_a_secure_query();
    how_to_make_an_unsecure_update();
    how_to_make_a_secure_update();

    how_to_make_an_unsecure_subscription();
    how_to_make_a_secure_subscription();

    how_to_unsubscribe();

    printf("Closing subscription channels...\n");
    close_subscription_channel(&sc);
    close_subscription_channel(&ssc);

    printf("Insert a number to end the program...\n\n"); scanf("%d", &i);
    return 0;
}

void how_to_make_an_unsecure_query() {
    HttpJsonResult result_data;
    long result_code;

    result_code = sepa_query_unsecure("select * where {?a ?b ?c}",
                                      "http://localhost:8000/query",
                                      &result_data);
    printf("---UNSECURE QUERY #1---\n");
    printf("Result code: %ld\n", result_code);
    if (result_code == 200) printf("Result data: %s\n", result_data.json);
    printf("---END---\n\n\n");

    result_code = sepa_query_all_unsecure("http://localhost:8000/query",
                                          &result_data);
    printf("---UNSECURE QUERY #2---\n");
    printf("Result code: %ld\n", result_code);
    if (result_code == 200) printf("Result data: %s\n", result_data.json);
    printf("---END---\n\n\n");
}


void how_to_make_a_secure_query() {
    sClient jwt = _init_sClient();
    HttpJsonResult result_data;
    long result_code=200;

    result_code = sepa_query("select * where {?a ?b ?c}",
                             "https://localhost:8443/secure/query",
                             "https://localhost:8443/oauth/register",
                             "https://localhost:8443/oauth/token",
                             "THISCLIENTID",
                             &jwt,
                             &result_data);
    printf("---SECURE QUERY #1---\n");
    printf("Result code: %ld\n", result_code);
    if (result_code == 200) printf("JWT=%s\nResult data:%s\n", jwt.JWT, result_data.json);
    printf("---END---\n\n\n");

    // This shows how to store an sClient information in a file for future usage.
    store_sClient("./data.jwt", jwt);

    result_code = sepa_query_all_secure("https://localhost:8443/secure/query",
                                        "THISCLIENTID",
                                        &jwt,
                                        &result_data);
    printf("---SECURE QUERY #2---\n");
    printf("Result code: %ld\n", result_code);
    if (result_code == 200) printf("JWT=%s\nResult data:%s\n", jwt.JWT, result_data.json);
    printf("---END---\n\n\n");
}


void how_to_make_an_unsecure_update() {
    long result_code;
    HttpJsonResult result_data;

    result_code = sepa_clear_unsecure("http://localhost:8000/update",
                                      &result_data);
    printf("---UNSECURE UPDATE #1---\n");
    printf("Result code: %ld\n", result_code);
    if (result_code == 200) printf("Result data: %s\n", result_data.json);
    printf("---END---\n\n\n");

    result_code = sepa_update_unsecure("insert data {<http://pippo.org> <http://paperino.org> <http://topolino.org>}",
                                       "http://localhost:8000/update",
                                       &result_data);
    printf("---UNSECURE UPDATE #2---\n");
    printf("Result code: %ld\n", result_code);
    if (result_code == 200) printf("Result data: %s\n", result_data.json);
    printf("---END---\n\n\n");
}


void how_to_make_a_secure_update() {
    long result_code;
    HttpJsonResult result_data;

    // This shows how to retrieve an sClient information from a file
    read_sClient("./data.jwt", &in_jwt);

    result_code = sepa_clear_secure("https://localhost:8443/secure/update",
                                    "THISCLIENTID",
                                    &in_jwt,
                                    &result_data);
    printf("---SECURE UPDATE #1---\n");
    printf("Result code: %ld\n", result_code);
    if (result_code == 200) printf("JWT=%s\nResult data:%s\n", in_jwt.JWT, result_data.json);
    printf("---END---\n\n\n");

    result_code = sepa_update_secure("insert data {<http://pippo.org> <http://paperino.org> <http://topolino.org>}",
                                     "https://localhost:8443/secure/update",
                                     "THISCLIENTID",
                                     &in_jwt,
                                     &result_data);
    printf("---SECURE UPDATE #2---\n");
    printf("Result code: %ld\n", result_code);
    if (result_code == 200) printf("JWT=%s\nResult data:%s\n", in_jwt.JWT, result_data.json);
    printf("---END---\n\n\n");
    sClient_free(&in_jwt);
}


void * mySubscriptionHandler(void *data) {
    printf("Received UNSECURE data %s\n", (char *) data);
    return NULL;
}

void * mySubscriptionHandler_secure(void *data) {
    printf("Received SECURE data %s\n", (char *) data);
    return NULL;
}

void how_to_make_an_unsecure_subscription() {
    int i, r;
    r = new_unsecure_channel("ws://localhost:9000/subscribe",
                         1,
                         &sc);

    if (!r) {
        mySub = sepa_subscribe_unsecure("select * where {?a ?b ?c}",
                                        "everything",
                                        mySubscriptionHandler,
                                        &sc);
        printf("Insert a number to continue...\n\n"); scanf("%d", &i);
    }
}

void how_to_make_a_secure_subscription() {
    int i, r;

    // This shows how to retrieve an sClient information from a file
    read_sClient("./data.jwt", &in_jwt);

    r = new_secure_channel("wss://localhost:9443/secure/subscribe",
                        1,
                        "THISCLIENTID",
                        &in_jwt,
                        &ssc);

    if (!r) {
        mysSub = sepa_subscribe_secure("select * where {?a ?b ?c}",
                                        "everything_secure",
                                        mySubscriptionHandler_secure,
                                        &ssc);
        printf("Insert a number to continue...\n\n"); scanf("%d", &i);
    }
}

void how_to_unsubscribe() {
    int i;
    sepa_unsubscribe(mySub);
    printf("Insert a number to continue...\n\n"); scanf("%d", &i);
    sepa_unsubscribe(mysSub);
    printf("Insert a number to close the subscription channels...\n\n"); scanf("%d", &i);
}
