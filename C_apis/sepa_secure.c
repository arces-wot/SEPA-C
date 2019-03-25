#include "sepa_secure.h"

void fprintf_sClient(FILE *destination, sClient jwt) {
    fprintf(destination, "{\"client_id\": \"%s\", \"client_secret\": \"%s\", \"JWT\": \"%s\", \"JWTtype\": \"%s\", \"expiration\": %ld}",
            jwt.client_id, jwt.client_secret, jwt.JWT, jwt.JWTtype, jwt.expiration);
}

int store_sClient(const char *path, sClient jwt) {
    FILE *jwtBin;
    char empty = '\n';
    char *error_string;

    jwtBin = fopen(path, "w+b");
    if (jwtBin == NULL) {
        error_string = strdup_format(path, "error writing jwt file %s - ");
        perror(error_string);
        free(error_string);
        return EXIT_FAILURE;
    }

    fwrite(&jwt, sizeof(sClient), 1, jwtBin);
    fwrite(jwt.client_id, sizeof(char), strlen(jwt.client_id), jwtBin);
    fwrite(&empty, sizeof(char), 1, jwtBin);
    fwrite(jwt.client_secret, sizeof(char), strlen(jwt.client_secret), jwtBin);
    fwrite(&empty, sizeof(char), 1, jwtBin);
    fwrite(jwt.JWT, sizeof(char), strlen(jwt.JWT), jwtBin);
    fwrite(&empty, sizeof(char), 1, jwtBin);
    fwrite(jwt.JWTtype, sizeof(char), strlen(jwt.JWTtype), jwtBin);
    fwrite(&empty, sizeof(char), 1, jwtBin);
    fclose(jwtBin);
    return EXIT_SUCCESS;
}

int read_sClient(const char *path, psClient jwt) {
    FILE *jwtBin;
    struct stat info;
    long fsize;
    char *buffer, *error_string;

    stat(path, &info);
    fsize = info.st_size;

    buffer = (char *) malloc(fsize*sizeof(char));
    assert(buffer);

    assert(jwt);
    jwtBin = fopen(path, "r+b");
    if (jwtBin == NULL) {
        error_string = strdup_format(path, "error opening jwt file %s - ");
        perror(error_string);
        free(error_string);
        return EXIT_FAILURE;
    }
    fread(jwt, sizeof(sClient), 1, jwtBin);

    fgets(buffer, fsize, jwtBin);
    buffer[strlen(buffer)-1] = '\0';
    jwt->client_id = strdup(buffer);

    fgets(buffer, fsize, jwtBin);
     buffer[strlen(buffer)-1] = '\0';
    jwt->client_secret = strdup(buffer);

    fgets(buffer, fsize, jwtBin);
     buffer[strlen(buffer)-1] = '\0';
    jwt->JWT = strdup(buffer);

    fgets(buffer, fsize, jwtBin);
    buffer[strlen(buffer)-1] = '\0';
    jwt->JWTtype = strdup(buffer);

    fclose(jwtBin);
    free(buffer);
    return EXIT_SUCCESS;
}

void sClient_free(psClient jwt) {
    if (jwt) {
        if (jwt->client_id) free(jwt->client_id);
        if (jwt->client_secret) free(jwt->client_secret);
        if (jwt->JWT) free(jwt->JWT);
        if (jwt->JWTtype) free(jwt->JWTtype);
        jwt->expiration = 0;
    }
}


long sepa_register(char *client_id,
                   const char *registration_request_url,
                   psClient jwt) {
    CURL *curl;
    CURLcode result;
    struct curl_slist *list = NULL;
    long response_code;
    HttpJsonResult data;
    char *requestBody;

    jsmn_parser parser;
    jsmntok_t *jstokens;
    int jstok_dim, parse_res;

    assert(client_id != NULL);
    assert(registration_request_url != NULL);
    assert(jwt != NULL);

    data.size = 0;
    data.json = (char *) malloc(REGISTRATION_BUFFER_LENGTH*sizeof(char));
    assert(data.json != NULL);

    assert(http_client_init() != EXIT_FAILURE);

    curl = curl_easy_init();
    assert(curl);
    curl_easy_setopt(curl, CURLOPT_URL, registration_request_url);

    requestBody = strdup_format(client_id, "{\"register\":{\"client_identity\":\"%s\",\"grant_types\":[\"client_credentials\"]}}");

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // CHECK HERE
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); // CHECK HERE
    list = curl_slist_append(list, "Content-Type: application/json");
    list = curl_slist_append(list, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, queryResultAccumulator);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    result = curl_easy_perform(curl);

    free(requestBody);
    if (result != CURLE_OK) {
        fprintf(stderr, "sepa_register curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
        freeHttpJsonResult(&data);
        return 1;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    #ifdef __SEPA_VERBOSE
    printf("sepa_register response code is %ld\n", response_code);
    #endif
    curl_easy_cleanup(curl);
    http_client_free();
    if (response_code != 201) {
        freeHttpJsonResult(&data);
        return response_code;
    }
    #ifdef __SEPA_VERBOSE
    printf("sepa_register response: %s\n", data.json);
    #endif

    jstok_dim = jsmn_getTokenLen(data.json, 0, data.size);
    if (jstok_dim < 0) {
        freeHttpJsonResult(&data);
        return 2;
    }

    jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
    assert(jstokens != NULL);

    jsmn_init(&parser);
    if (jsmn_parse(&parser, data.json, data.size, jstokens, jstok_dim)<0) {
        freeHttpJsonResult(&data);
        free(jstokens);
        return 3;
    }

    parse_res = jsmn_explore(data.json, &(jwt->client_id),
                     jstokens, jstok_dim, 2,
                     "credentials", "client_id");
    if (parse_res < 0) {
        fprintf(stderr, "Unable to jsmn_explore() for client_id (code %d)\n", parse_res);
        freeHttpJsonResult(&data);
        free(jstokens);
        return 4;
    }
    #ifdef __SEPA_VERBOSE
    printf("Cliend id: %s\n", jwt->client_id);
    #endif // __SEPA_VERBOSE

    parse_res = jsmn_explore(data.json, &(jwt->client_secret),
                     jstokens, jstok_dim, 2,
                     "credentials", "client_secret");
    if (parse_res < 0) {
        fprintf(stderr, "Unable to jsmn_explore() for client_secret (code %d)\n", parse_res);
        freeHttpJsonResult(&data);
        free(jstokens);
        return 5;
    }
    #ifdef __SEPA_VERBOSE
    printf("Client secret: %s\n", jwt->client_secret);
    #endif // __SEPA_VERBOSE

    freeHttpJsonResult(&data);
    free(jstokens);
    return response_code;
}


long sepa_request_token(const char *token_request_url, psClient jwt) {
    CURL *curl;
    CURLcode result;
    struct curl_slist *list = NULL;
    long response_code;
    HttpJsonResult data;
    char *ascii_key, *b64_key, *jwt_expiration, *next;

    jsmn_parser parser;
    jsmntok_t *jstokens;
    int jstok_dim;

    assert(token_request_url != NULL);
    assert(jwt != NULL);

    if ((jwt->client_id == NULL) || (jwt->client_secret == NULL)) {
        fprintf(stderr, "Both client_id{%s} and client_secret{%s} must be non NULL\n", jwt->client_id, jwt->client_secret);
        return 1;
    }

    ascii_key = (char *) malloc((strlen(jwt->client_id)+strlen(jwt->client_secret)+3)*sizeof(char));
    assert(ascii_key);
    sprintf(ascii_key, "%s:%s", jwt->client_id, jwt->client_secret);
    b64_key = b64_encode(ascii_key);
    #ifdef __SEPA_VERBOSE
    printf("JWT: base64(%s) = %s\n", ascii_key, b64_key);
    #endif
    free(ascii_key);

    assert(http_client_init() != EXIT_FAILURE);
    curl = curl_easy_init();
    assert(curl);
    curl_easy_setopt(curl, CURLOPT_URL, token_request_url);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // CHECK HERE
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); // CHECK HERE

    ascii_key = strdup_format(b64_key, "Authorization: Basic %s");
    #ifdef __SEPA_VERBOSE
    printf("Ascii_key = %s\n", ascii_key);
    #endif
    list = curl_slist_append(list, "Content-Type: application/json");
    list = curl_slist_append(list, "Accept: application/json");
    if (*ascii_key != '\0') list = curl_slist_append(list, ascii_key);

    data.size = 0;
    data.json = (char *) malloc(REGISTRATION_BUFFER_LENGTH*sizeof(char));
    assert(data.json != NULL);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, queryResultAccumulator);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    result = curl_easy_perform(curl);
    free(ascii_key);
    if (result != CURLE_OK) {
        fprintf(stderr, "sepa_request_token curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
        freeHttpJsonResult(&data);
        return 2;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    #ifdef __SEPA_VERBOSE
    printf("sepa_request_token response code is %ld\n", response_code);
    #endif
    curl_easy_cleanup(curl);
    http_client_free();
    if (response_code != 201) {
        freeHttpJsonResult(&data);
        return response_code;
    }

    jstok_dim = jsmn_getTokenLen(data.json, 0, data.size);
    if (jstok_dim<0) {
        freeHttpJsonResult(&data);
        return 3;
    }

    jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
    assert(jstokens != NULL);

    jsmn_init(&parser);
    if (jsmn_parse(&parser, data.json, data.size, jstokens, jstok_dim)<0) {
        freeHttpJsonResult(&data);
        free(jstokens);
        return 4;
    }

    if (jsmn_explore(data.json, &(jwt->JWT),
                 jstokens, jstok_dim, 2,
                 "token", "access_token") < 0) {
        fprintf(stderr, "Unable to jsmn_explore() for JWT\n");
        freeHttpJsonResult(&data);
        free(jstokens);
        return 5;
    }
    #ifdef __SEPA_VERBOSE
    fprintf(stderr, "JWT: %s\n", jwt->JWT);
    #endif // __SEPA_VERBOSE

    if (jsmn_explore(data.json, &(jwt->JWTtype),
                 jstokens, jstok_dim, 2,
                 "token", "token_type") < 0) {
        fprintf(stderr, "Unable to jsmn_explore() for JWT type\n");
        freeHttpJsonResult(&data);
        free(jstokens);
        return 6;
    }
    #ifdef __SEPA_VERBOSE
    fprintf(stderr, "JWT type: %s\n", jwt->JWTtype);
    #endif // __SEPA_VERBOSE

    if (jsmn_explore(data.json, &jwt_expiration,
                 jstokens, jstok_dim, 2,
                 "token", "token_type") < 0) {
        fprintf(stderr, "Unable to jsmn_explore() for JWT type\n");
        freeHttpJsonResult(&data);
        free(jstokens);
        return 7;
    }
    jwt->expiration = strtol(jwt_expiration, &next, 10);
    #ifdef __SEPA_VERBOSE
    fprintf(stderr, "JWT expiration: %ld\n", jwt->expiration);
    #endif
    freeHttpJsonResult(&data);
    free(jstokens);
    return response_code;
}
