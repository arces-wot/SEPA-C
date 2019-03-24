#include "sepa_utils.h"

static volatile int CURL_INITIALIZED = 0;

int http_client_init() {
    char executed[15] = "(Nothing done)";
    if (CURL_INITIALIZED>=0) {
        if (!CURL_INITIALIZED) {
            strcpy(executed, "(curl init)");
            if (curl_global_init(CURL_GLOBAL_ALL)) {
                fprintf(stderr, "curl_global_init() failed!");
                return EXIT_FAILURE;
            }
        }
        CURL_INITIALIZED++;
        #ifdef __SEPA_VERBOSE
        printf("http_client_init() completed successfully: %d active clients %s\n", CURL_INITIALIZED, executed);
        #endif
    }
    return EXIT_SUCCESS;
}

void http_client_free() {
    char executed[15] = "(Nothing done)";
    if (CURL_INITIALIZED>0) {
        if (CURL_INITIALIZED == 1) {
            strcpy(executed, "(curl ended)");
            curl_global_cleanup();
        }
        CURL_INITIALIZED--;
        #ifdef __SEPA_VERBOSE
        printf("http_client_free() completed successfully: %d active clients %s\n", CURL_INITIALIZED, executed);
        #endif
    }
}

void freeHttpJsonResult(pHttpJsonResult jresult) {
    jresult->size = 0;
    free(jresult->json);
}

size_t queryResultAccumulator(void *ptr,
                              size_t size,
                              size_t nmemb,
                              pHttpJsonResult data) {
    size_t index = data->size;
    size_t n = size*nmemb;
    char *tmp;

    data->size += size*nmemb;
    tmp = realloc(data->json, data->size+1); // +1 for '\0'
    if (tmp != NULL) data->json = tmp;
    else {
        if (data->json != NULL) freeHttpJsonResult(data);
        fprintf(stderr, "Failed to allocate memory in queryResultAccumulator.\n");
        return 0;
    }

    memcpy(data->json+index, ptr, n);
    data->json[data->size] = '\0';
    return size*nmemb;
}

char * strdup_format(char *origin, const char *format) {
    char *result;
    if (origin) {
        result = (char *) malloc((strlen(origin)+strlen(format)+3)*sizeof(char));
        assert(result);
        sprintf(result, format, origin);
    }
    else result = strdup("");
    return result;
}
