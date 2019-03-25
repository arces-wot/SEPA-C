#include "subscription.h"

static pthread_mutex_t global_sub_mutex = PTHREAD_MUTEX_INITIALIZER;
static pSubscriptionChannel waiting_channels[MAX_WAITING_CHANNELS] = {NULL, NULL, NULL, NULL, NULL};

void freeSubscription(pSubscription sub) {
    assert(sub);
    free(sub->sparql);
    free(sub->alias);
    free(sub->spuid);
    *sub = _init_subscription();
}

int explore_result(pSubscriptionChannel ch, jsmntok_t *jstokens, int jstok_dim) {
    int situation, i;
    char *spuid, *alias;

    jsmn_explore(ch->r_buffer, &spuid, jstokens, jstok_dim, 2, "unsubscribed", "spuid");
    if (spuid != NULL) {
        #ifdef __SEPA_VERBOSE
        printf("Unsubscribe confirm for %s\n", spuid);
        #endif // __SEPA_VERBOSE
        situation = UNSUBSCRIBE;
    }
    else {
        jsmn_explore(ch->r_buffer, &spuid, jstokens, jstok_dim, 2, "notification", "spuid");
        if (spuid != NULL) {
            jsmn_explore(ch->r_buffer, &alias, jstokens, jstok_dim, 2, "notification", "alias");
            if (alias != NULL) situation = FIRST_NOTIFICATION;
            else situation = LATER_NOTIFICATION;
        }
        else return ERROR_EXPLORING_RESULT;
    }
    for (i=0; i<ch->n_sub; i++) {
        switch (situation) {
        case FIRST_NOTIFICATION:
            if (!strcmp((ch->subs)[i].alias, alias)) {
                (ch->subs)[i].spuid = strdup(spuid);
                assert((ch->subs)[i].spuid);
                return i;
            }
            break;
        case LATER_NOTIFICATION:
            if (!strcmp((ch->subs)[i].spuid, spuid)) return i;
            break;
        default:
            if (((ch->subs)[i].spuid != NULL) && (!strcmp((ch->subs)[i].spuid, spuid))) {
                freeSubscription(&((ch->subs)[i]));
                return UNSUBSCRIBE;
            }
            break;
        }
    }
    return ERROR_EXPLORING_RESULT;
}

static int channel_callback(struct lws *wsi,
                          enum lws_callback_reasons reason,
                          void *user,
                          void *in,
                          size_t len) {
    jsmn_parser parser;
    jsmntok_t *jstokens;
    int jstok_dim, res, i;
    char *_result;
    pthread_t _handler;

    pSubscriptionChannel channel = (pSubscriptionChannel) user;
    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:

        assert(!pthread_mutex_lock(&global_sub_mutex));
        for (i=0; i<=MAX_WAITING_CHANNELS; i++) {
            if (i == MAX_WAITING_CHANNELS) {
                fprintf(stderr, "Max number of waiting channels reached\n");
                assert(0);
            }
            if (wsi == waiting_channels[i]->ws_id) {
                *channel = *(waiting_channels[i]);
                pthread_cond_broadcast(&(waiting_channels[i]->connected));
                waiting_channels[i] = NULL;
                #ifdef __SEPA_VERBOSE
                printf("Connection established (%d)!\n%d waiting channels\n", reason, i);
                #endif // __SEPA_VERBOSE
                break;
            }
        }
        pthread_mutex_unlock(&global_sub_mutex);
        channel->r_buffer = strdup("");
        assert(channel->r_buffer);
        break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        fprintf(stderr, "Sepa Callback: Connect with server error.: %s\n", (char *) in);
        assert(!pthread_mutex_lock(&global_sub_mutex));
        for (i=0; i<MAX_WAITING_CHANNELS; i++) {
            if (wsi == waiting_channels[i]->ws_id) {
                pthread_cond_broadcast(&(waiting_channels[i]->connected));
                assert(!pthread_mutex_lock(&(waiting_channels[i]->ws_mutex)));
                free(waiting_channels[i]->host);
                waiting_channels[i]->host = NULL;
                pthread_mutex_unlock(&(waiting_channels[i]->ws_mutex));
                break;
            }
        }
        pthread_mutex_unlock(&global_sub_mutex);
        break;
    case LWS_CALLBACK_CLIENT_RECEIVE:
        if ((channel->r_buffer == NULL) || (len<=0)) break;
        #ifdef __SEPA_VERBOSE
        printf("Websocket buffer in: %s\n", (char *) in);
        #endif
        assert(!pthread_mutex_lock(&(channel->ws_mutex)));
        channel->r_buffer = (char *) realloc(channel->r_buffer, (strlen(channel->r_buffer)+len+2)*sizeof(char));
        assert(channel->r_buffer);
        strncat(channel->r_buffer, (char *) in, len);

        jstok_dim = jsmn_getTokenLen(channel->r_buffer, 0, strlen(channel->r_buffer));
        if (jstok_dim>0) {
            jstokens = (jsmntok_t *) malloc(jstok_dim*sizeof(jsmntok_t));
            assert(jstokens);
            jsmn_init(&parser);
            if (jsmn_parse(&parser, channel->r_buffer, strlen(channel->r_buffer), jstokens, jstok_dim)>0) {
                res = explore_result(channel, jstokens, jstok_dim);
                switch (res) {
                case UNSUBSCRIBE:
                    #ifdef __SEPA_VERBOSE
                    printf("Unsubscribe packet detected\n");
                    #endif // __SEPA_VERBOSE
                    break;
                case ERROR_EXPLORING_RESULT:
                    fprintf(stderr, "Error in exploring the result %s\n", channel->r_buffer);
                    break;
                default:
                    #ifdef __SEPA_VERBOSE
                    printf("Notification packet detected\n");
                    #endif // __SEPA_VERBOSE
                    _result = strdup(channel->r_buffer);
                    assert(_result);
                    assert(!pthread_create(&_handler, NULL, (channel->subs)[res].handler, (void *) _result));
                    break;
                }
                strcpy(channel->r_buffer, "");
            }
        }
        pthread_mutex_unlock(&(channel->ws_mutex));
        break;
    default:
        break;
    }
    return 0;
}

void * channel_function(void *parameters) {
    struct lws_context_creation_info lwsContext_info;
    struct lws_context *ws_context;
    struct lws_client_connect_info connect_info;
    struct lws_protocols _protocol;
    const char *protocol, *address, *path;
    char *host_copy, *real_path;
    int port, keep_ws_alive=1, i;

    pSubscriptionChannel channel = (pSubscriptionChannel) parameters;
    _protocol.name = strdup("SPARQL 1.1 Subscribe Language");
    _protocol.callback = channel_callback;
    _protocol.user = parameters;
    _protocol.per_session_data_size = sizeof(SubscriptionChannel);
    _protocol.rx_buffer_size = RX_BUFFER_SIZE;

    memset(&lwsContext_info, 0, sizeof lwsContext_info);
    memset(&connect_info, 0, sizeof connect_info);

    host_copy = strdup(channel->host);
    assert(host_copy);
    assert(!lws_parse_uri(host_copy, &protocol, &address, &port, &path));
    real_path = (char *) malloc((strlen(path)+3)*sizeof(char));
    assert(real_path);
    sprintf(real_path, "/%s", path);
    #ifdef __SEPA_VERBOSE
    printf("Protocol: %s, Address: %s, Port: %d, Path: %s\n", protocol, address, port, real_path);
    #endif // __SEPA_VERBOSE

    lwsContext_info.port = CONTEXT_PORT_NO_LISTEN;
    lwsContext_info.protocols = &_protocol;
    lwsContext_info.extensions = NULL;
    lwsContext_info.gid = -1;
    lwsContext_info.uid = -1;
    lwsContext_info.ws_ping_pong_interval = 0;
    lwsContext_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED;
    ws_context = lws_create_context(&lwsContext_info);
    assert(ws_context != NULL);
    #ifdef __SEPA_VERBOSE
    printf("lws_create_context() passed successfully\n");
    #endif // __SEPA_VERBOSE

    connect_info.context = ws_context;
    connect_info.address = address;
    connect_info.port = port;
    connect_info.path = real_path;

    if (channel->jwt == NULL) connect_info.ssl_connection = 0;
    else connect_info.ssl_connection = LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK | LCCSCF_USE_SSL | LCCSCF_ALLOW_EXPIRED;

    connect_info.host = connect_info.address;
    connect_info.origin = connect_info.address;
    connect_info.ietf_version_or_minus_one = -1;
    connect_info.pwsi = &(channel->ws_id);
    channel->ws_id = lws_client_connect_via_info(&connect_info);

    assert(!pthread_mutex_lock(&global_sub_mutex));
    for (i=0; i<=MAX_WAITING_CHANNELS; i++) {
        if (i == MAX_WAITING_CHANNELS) {
            fprintf(stderr, "Max number of waiting channels reached\n");
            assert(0);
        }
        if (waiting_channels[i]==NULL) {
            waiting_channels[i] = channel;
            break;
        }
    }
    #ifdef __SEPA_VERBOSE
    printf("Channel subs (thread)=%p\nlws_client_connect_via_info() success: %p\n", channel->subs, channel->ws_id);
    #endif // __SEPA_VERBOSE
    pthread_mutex_unlock(&global_sub_mutex);

    while (keep_ws_alive) {
        assert(!pthread_mutex_lock(&(channel->ws_mutex)));
        keep_ws_alive = (channel->host != NULL);
        pthread_mutex_unlock(&(channel->ws_mutex));
        lws_service(ws_context, 0);
    }
    lws_context_destroy(ws_context);
    pthread_mutex_destroy(&(channel->ws_mutex));
    #ifdef __SEPA_VERBOSE
    printf("Channel thread ended!\n");
    #endif // __SEPA_VERBOSE
    return EXIT_SUCCESS;
}

int open_subscription_channel(const char *host,
                              int n_sub,
                              const char *registration_request_url,
                              const char *token_request_url,
                              const char *client_id,
                              psClient jwt,
                              pSubscriptionChannel channel) {
    pthread_t channel_thread;
    int i, result=0;
    long output_code;
    pSubscription _subs;

    assert(host);
    assert(n_sub>0);
    assert(channel);

    #ifdef __SEPA_VERBOSE
    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO, NULL);
    #else
    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);
    #endif // __SEPA_VERBOSE

    channel->connected = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    assert(!pthread_mutex_lock(&global_sub_mutex));

    channel->host = strdup(host);
    assert(channel->host);

    channel->n_sub = n_sub;
    channel->subs = (pSubscription) malloc(n_sub*sizeof(Subscription));
    assert(channel->subs);
    _subs = channel->subs;
    for (i=0; i<n_sub; i++) {
        _subs[i] = _init_subscription();
        _subs[i].channel = channel;
    }

    channel->jwt = jwt;
    if (jwt) {
        if (jwt->client_secret == NULL) {
            #ifdef __SEPA_VERBOSE
            printf("JWT client_secret not available: registering...\n");
            #endif // __SEPA_VERBOSE
            assert(client_id != NULL);
            assert(registration_request_url != NULL);
            output_code = sepa_register(client_id, registration_request_url, jwt);
            if (output_code != 201) {
                fprintf(stderr, "Registration output code %ld (failure)\n", output_code);
                result = 1;
            }
        }
        if (jwt->JWT == NULL) {
            #ifdef __SEPA_VERBOSE
            printf("JWT token not available: requesting token...\n");
            #endif // __SEPA_VERBOSE
            assert(token_request_url != NULL);
            output_code = sepa_request_token(token_request_url, jwt);
            if (output_code != 201) {
                fprintf(stderr, "Token request output code %ld (failure)", output_code);
                result = 2;
            }
        }
    }
    channel->r_buffer = NULL;
    channel->ws_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    if (!result) {
        assert(!pthread_create(&channel_thread, NULL, channel_function, (void *) channel));
        pthread_cond_wait(&(channel->connected), &global_sub_mutex);
    }
    else fprintf(stderr, "Error while getting secure authorization, channel creation aborted.\n");

    pthread_mutex_unlock(&global_sub_mutex);
    if (channel->host == NULL) {
        fprintf(stderr, "Error detected in creating ws channel for '%s'.\n", host);
        result = 3;
    }
    #ifdef __SEPA_VERBOSE
    else printf("Channel '%s' successfully created!\n", host);
    #endif // __SEPA_VERBOSE
    return result;
}

static void send_text_ws(struct lws *wsi, char *buffer) {
    unsigned char *t_buffer = (unsigned char *) buffer;
    uint8_t flags = LWS_WRITE_TEXT;
    int chars_sent = 0;
    int remaining = strlen(buffer);
    clock_t before;

    do {
        t_buffer += chars_sent;
        if (remaining > CHUNK_MAX_SIZE) flags |= LWS_WRITE_NO_FIN;
        before = clock();
        while (lws_send_pipe_choked(wsi) && (((clock()-before)/CLOCKS_PER_SEC)<LWS_PIPE_CHOKED_TIMEOUT));
        if (CHUNK_MAX_SIZE < remaining) chars_sent = lws_write(wsi, t_buffer, CHUNK_MAX_SIZE, flags);
        else chars_sent = lws_write(wsi, t_buffer, remaining, flags);
        remaining -= chars_sent;
        flags = LWS_WRITE_CONTINUATION;
    } while (remaining > 0);
}

pSubscription sepa_subscribe(const char *sparql,
                             const char *alias,
                             const char *d_graph,
                             const char *n_graph,
                             SubscriptionHandler handler,
                             pSubscriptionChannel channel) {
    char packet_format[] = "{\"subscribe\": {\"sparql\": \"%s\", \"alias\": \"%s\"%s%s%s}}";
    int i;
    pSubscription _sub;
    char *packet, *dg, *ng, *auth, *_jwt;

    assert(sparql);
    assert(alias);
    assert(handler);
    assert(channel);

    assert(!pthread_mutex_lock(&(channel->ws_mutex)));
    for (i=0; i<=channel->n_sub; i++) {
        if (i == channel->n_sub) {
            fprintf(stderr, "Channel already contains %d subscriptions\n", i);
            return NULL;
        }
        if ((channel->subs)[i].handler == NULL) {
            _sub = &((channel->subs)[i]);
            break;
        }
    }

    _sub->handler = handler;
    _sub->alias = strdup(alias);
    _sub->sparql = strdup(sparql);
    assert(_sub->alias);
    assert(_sub->sparql);

    if (channel->jwt == NULL) _jwt = NULL;
    else _jwt = channel->jwt->JWT;
    auth = strdup_format(_jwt, ", \"authorization\": \"Bearer %s\"");
    dg = strdup_format(d_graph, ", \"default-graph-uri\": \"%s\"");
    ng = strdup_format(n_graph, ", \"named-graph-uri\": \"%s\"");

    packet = (char *) malloc((strlen(packet_format)+strlen(sparql)+strlen(alias)+strlen(auth)+strlen(dg)+strlen(ng)+3+LWS_PRE)*sizeof(char));
    assert(packet);
    sprintf(packet+LWS_PRE, packet_format, sparql, alias, auth, dg, ng);
    #ifdef __SEPA_VERBOSE
    printf("Subscription packet: %s\n", packet+LWS_PRE);
    #endif // __SEPA_VERBOSE
    send_text_ws(channel->ws_id, packet+LWS_PRE);
    pthread_mutex_unlock(&(channel->ws_mutex));

    free(packet);
    free(auth);
    free(dg);
    free(ng);
    return _sub;
}

int sepa_unsubscribe(pSubscription subscription) {
    char packet_format[] = "{\"unsubscribe\": {\"spuid\": \"%s\"%s}}";
    char *packet, *auth, *_jwt;
    if ((subscription != NULL) && (subscription->channel == NULL)) {
        #ifdef __SEPA_VERBOSE
        printf("Null subscription parameter: ignoring call\n");
        #endif // __SEPA_VERBOSE
        return 1;
    }
    #ifdef __SEPA_VERBOSE
    printf("Called unsubscribe for '%s' subscription (spuid=%s)\n", subscription->alias, subscription->spuid);
    #endif // __SEPA_VERBOSE

    if (subscription->channel->jwt == NULL) _jwt = NULL;
    else _jwt = subscription->channel->jwt->JWT;
    auth = strdup_format(_jwt, ", \"authorization\": \"Bearer %s\"");
    packet = (char *) malloc((strlen(packet_format)+strlen(subscription->spuid)+strlen(auth)+3+LWS_PRE)*sizeof(char));
    assert(packet);
    sprintf(packet+LWS_PRE, packet_format, subscription->spuid, auth);
    #ifdef __SEPA_VERBOSE
    printf("Unsubscription packet: %s\n", packet+LWS_PRE);
    #endif // __SEPA_VERBOSE
    send_text_ws(subscription->channel->ws_id, packet+LWS_PRE);
    free(auth);
    free(packet);
    return 0;
}

void close_subscription_channel(pSubscriptionChannel channel) {
    int i;
    if (channel) {
        assert(!pthread_mutex_lock(&(channel->ws_mutex)));
        for (i=0; i<channel->n_sub; i++) {
            sepa_unsubscribe(&((channel->subs)[i]));
        }
        free(channel->host);
        free(channel->r_buffer);
        channel->host = NULL;
        pthread_cond_destroy(&(channel->connected));
        sClient_free(channel->jwt);
        pthread_mutex_unlock(&(channel->ws_mutex));
    }
}
