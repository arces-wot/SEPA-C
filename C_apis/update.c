#include "update.h"

long sepa_update(const char *sparql_update,
                 const char *host,
                 const char *registration_request_url,
                 const char *token_request_url,
                 const char *client_id,
                 psClient jwt,
                 pHttpJsonResult result_data) {
    CURL *curl;
    CURLcode result;
    struct curl_slist *list = NULL;
    long response_code = 0;
    char *auth_header;
    HttpJsonResult temp_result;

    assert(sparql_update != NULL);
    assert(host != NULL);

    temp_result.size = 0;
    temp_result.json = (char *) malloc(UPDATE_RESULT_BUFFER_LENGTH*sizeof(char));
    assert(temp_result.json != NULL);

    assert(http_client_init() != EXIT_FAILURE);
    curl = curl_easy_init();
    assert(curl);
    curl_easy_setopt(curl, CURLOPT_URL, host);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sparql_update);

    if (jwt != NULL) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // CHECK HERE
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); // CHECK HERE
        if (jwt->client_secret == NULL) {
            #ifdef __SEPA_VERBOSE
            printf("JWT client_secret not available: registering...\n");
            #endif // __SEPA_VERBOSE
            assert(client_id != NULL);
            assert(registration_request_url != NULL);
            response_code = sepa_register(client_id, registration_request_url, jwt);
            if (response_code != 201) {
                fprintf(stderr, "Registration request code %ld (failure)\n", response_code);
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
            response_code = sepa_request_token(token_request_url, jwt);
            if (response_code != 201) {
                fprintf(stderr, "Token request code %ld (failure)\n", response_code);
                curl_easy_cleanup(curl);
                http_client_free();
                return 2;
            }
        }

        auth_header = strdup_format(jwt->JWT, "Authorization: Bearer %s");
        list = curl_slist_append(list, auth_header);
    }

    list = curl_slist_append(list, "Content-Type: application/sparql-update");
    list = curl_slist_append(list, "Accept: application/sparql-results+json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, queryResultAccumulator);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &temp_result);

    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
        response_code = 3;
    }
    else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (result_data != NULL) {
            result_data->size = temp_result.size;
            result_data->json = strdup(temp_result.json);
            assert(result_data->json != NULL);
        }
    }
    curl_easy_cleanup(curl);
    http_client_free();
	if (jwt != NULL) free(auth_header);
    return response_code;
}
