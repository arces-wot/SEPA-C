#include "query.h"

long sepa_query(const char *sparql_query,
                const char *host,
                const char *registration_request_url,
                const char *token_request_url,
                char *client_id,
                psClient jwt,
                pHttpJsonResult result_data) {
    CURL *curl;
    CURLcode result;
    struct curl_slist *list = NULL;
    long return_code=0;
    char *auth_header;

    assert(sparql_query != NULL);
    assert(host != NULL);
    assert(result_data != NULL);

    result_data->size = 0;
    result_data->json = (char *) malloc(QUERY_START_BUFFER_LENGTH*sizeof(char));
    assert(result_data->json != NULL);

    assert(http_client_init() != EXIT_FAILURE);

    curl = curl_easy_init();
    assert(curl);
    curl_easy_setopt(curl, CURLOPT_URL, host);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sparql_query);
    if (jwt != NULL) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // CHECK HERE
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); // CHECK HERE
        if (jwt->client_secret == NULL) {
            #ifdef __SEPA_VERBOSE
            printf("JWT client_secret not available: registering...\n");
            #endif // __SEPA_VERBOSE
            assert(client_id != NULL);
            assert(registration_request_url != NULL);
            return_code = sepa_register(client_id, registration_request_url, jwt);
            if (return_code != 201) {
                fprintf(stderr, "Registration request code %ld (failure)\n", return_code);
                curl_easy_cleanup(curl);
                http_client_free();
                return 1;
            }
        }

        if (jwt->JWT == NULL) {
            #ifdef __SEPA_VERBOSE
            printf("JWT token not available: requesting token...\n");
            #endif // __SEPA_VERBOSE
            assert(token_request_url != NULL);
            return_code = sepa_request_token(token_request_url, jwt);
            if (return_code != 201) {
                fprintf(stderr, "Token request code %ld (failure)\n", return_code);
                curl_easy_cleanup(curl);
                http_client_free();
                return 2;
            }
        }

        auth_header = strdup_format(jwt->JWT, "Authorization: Bearer %s");
        list = curl_slist_append(list, auth_header);
    }

    list = curl_slist_append(list, "Content-Type: application/sparql-query");
    list = curl_slist_append(list, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, queryResultAccumulator);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, result_data);
    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
        return_code = 3;
    }
    else curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &return_code);
    curl_easy_cleanup(curl);
    http_client_free();
    if (jwt != NULL) free(auth_header);
    return return_code;
}
