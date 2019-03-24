#include "SEPA.h"

using namespace sepa;

const string sepa::uuidGenerator() {
    #ifdef WIN32
    UUID uuid;
    unsigned char *str;

    UuidCreate(&uuid);
    UuidToStringA(&uuid, &str);

    std::string s((char* ) str);
    RpcStringFreeA ( &str );
    return s;
    #else
    uuid_t uuid;
    char s[37];

    uuid_generate_random(uuid);
    uuid_unparse(uuid, s);
    return string(s);
    #endif
}

SEPA::SEPA(string client_id, ysap::YSAPObject sap_object) {
    this->client_id = client_id;
    this->sap_object = sap_object;
    this->secure_client = _init_sClient();
}

SEPA::SEPA(ysap::YSAPObject sap_object) {
    this->client_id = uuidGenerator();
    this->sap_object = sap_object;
    this->secure_client = _init_sClient();
}

SEPA::SEPA(string client_id) {
    this->client_id = client_id;
    this->sap_object = ysap::YSAPObject();
    this->secure_client = _init_sClient();
}

SEPA::SEPA() {
    this->client_id = uuidGenerator();
    this->sap_object = ysap::YSAPObject();
    this->secure_client = _init_sClient();
}

SEPA::~SEPA() {
    //dtor
}

void SEPA::setClientId(string client_id) {
    this->client_id = client_id;
}

const string SEPA::getClientId() {
    return this->client_id;
}

void SEPA::setSapObject(ysap::YSAPObject sap_object) {
    this->sap_object = sap_object;
}

ysap::YSAPObject SEPA::getSapObject() {
    return this->sap_object;
}

const long SEPA::query_unsecure(std::string identifier,
                                YAML::Node bindings,
                                std::ostream *destination,
                                std::string host) {
    auto sparql = this->getSapObject().getQuery(identifier, bindings, true);
    return this->sparql_query_unsecure(sparql, destination, host);
}

const long SEPA::sparql_query_unsecure(std::string sparql,
                                       std::ostream *destination,
                                       std::string host) {
    std::string real_host;
    if (!host.empty()) real_host = host;
    else real_host = this->getSapObject().query_url();

    HttpJsonResult data;
    long result = sepa_query_unsecure(sparql.c_str(),
                                      real_host.c_str(),
                                      &data);
    if (result == 200) *destination << std::string(data.json);
    return result;
}

const long SEPA::query_secure(std::string identifier,
                        YAML::Node bindings,
                        std::ostream *destination,
                        std::string host,
                        std::string registration_url,
                        std::string token_url) {
    auto sparql = this->getSapObject().getQuery(identifier, bindings, true);
    return this->sparql_query_secure(sparql, destination, host, registration_url, token_url);
}

const long SEPA::sparql_query_secure(std::string sparql,
                                     std::ostream *destination,
                                     std::string host,
                                     std::string registration_url,
                                     std::string token_url) {
    std::string real_host;
    std::string real_reg_url;
    std::string real_tok_url;

    if (!host.empty()) real_host = host;
    else real_host = this->getSapObject().query_url();

    if (!registration_url.empty()) real_reg_url = registration_url;
    else real_reg_url = this->getSapObject().registration_url();

    if (!token_url.empty()) real_tok_url = token_url;
    else real_tok_url = this->getSapObject().tokenRequest_url();

    HttpJsonResult data;
    long result = sepa_query(sparql.c_str(),
                             real_host.c_str(),
                             real_reg_url.c_str(),
                             real_tok_url.c_str(),
                             this->client_id.c_str(),
                             &(this->secure_client),
                             &data);
    if (result == 200) *destination << std::string(data.json);
    return result;
}


const void SEPA::storeSecureClient(std::string filepath) {
    store_sClient(filepath.c_str(), this->secure_client);
}

const void SEPA::setSecureClient(std::string filepath) {
    sClient_free(&(this->secure_client));
    read_sClient(filepath.c_str(), &(this->secure_client));
}

const long SEPA::update_unsecure(std::string identifier,
                           YAML::Node bindings,
                           std::ostream *destination,
                           std::string host) {
    auto sparql = this->getSapObject().getUpdate(identifier, bindings, true);
    return this->sparql_update_unsecure(sparql, destination, host);
}

const long SEPA::sparql_update_unsecure(std::string sparql,
                                  std::ostream *destination,
                                  std::string host) {
    std::string real_host;
    if (!host.empty()) real_host = host;
    else real_host = this->getSapObject().update_url();

    HttpJsonResult data;
    long result = sepa_update_unsecure(sparql.c_str(),
                                       real_host.c_str(),
                                       &data);

    if ((result == 200) && (destination != nullptr)) *destination << std::string(data.json);
    return result;
}

const long SEPA::update_secure(std::string identifier,
                         YAML::Node bindings,
                         std::ostream *destination,
                         std::string host,
                         std::string registration_url,
                         std::string token_url) {
    auto sparql = this->getSapObject().getUpdate(identifier, bindings, true);
    return this->sparql_update_secure(sparql, destination, host, registration_url, token_url);
}

const long SEPA::sparql_update_secure(std::string sparql,
                                std::ostream *destination,
                                std::string host,
                                std::string registration_url,
                                std::string token_url) {
    std::string real_host;
    std::string real_reg_url;
    std::string real_tok_url;

    if (!host.empty()) real_host = host;
    else real_host = this->getSapObject().update_url();

    if (!registration_url.empty()) real_reg_url = registration_url;
    else real_reg_url = this->getSapObject().registration_url();

    if (!token_url.empty()) real_tok_url = token_url;
    else real_tok_url = this->getSapObject().tokenRequest_url();

    HttpJsonResult data;
    long result = sepa_update(sparql.c_str(),
                              real_host.c_str(),
                              real_reg_url.c_str(),
                              real_tok_url.c_str(),
                              this->client_id.c_str(),
                              &(this->secure_client),
                              &data);
    if ((result == 200) && (destination != nullptr)) *destination << std::string(data.json);
    return result;
}

void SEPA::subscribe(std::string identifier,
                       YAML::Node bindings,
                       std::string alias,
                       SubscriptionHandler handler,
                       pSubscriptionChannel channel,
                       std::string default_graph,
                       std::string named_graph) {
    assert(handler);
    assert(channel);

    auto sparql = this->getSapObject().getQuery(identifier, bindings, true);
    auto graphs = this->getSapObject().graphs();
    const char *d_graph, *n_graph;
    if (default_graph.empty()) {
        if (graphs["default-graph-uri"]) d_graph = graphs["default-graph-uri"].as<string>().c_str();
        else d_graph = NULL;
    }
    else d_graph = default_graph.c_str();
    if (named_graph.empty()) {
        if (graphs["named-graph-uri"]) n_graph = graphs["named-graph-uri"].as<string>().c_str();
        else n_graph = NULL;
    }
    else n_graph = named_graph.c_str();

    this->subscriptions.push_back(sepa_subscribe(sparql.c_str(),
                                                 alias.c_str(),
                                                 d_graph,
                                                 n_graph,
                                                 handler,
                                                 channel));
}

void SEPA::sparql_subscribe(std::string sparql,
                            std::string alias,
                            SubscriptionHandler handler,
                            pSubscriptionChannel channel) {
    assert(handler);
    assert(channel);

    const char *d_graph, *n_graph;
    if (this->getSapObject().isEmpty()) {
        d_graph = NULL;
        n_graph = NULL;
    }
    else {
        auto graphs = this->getSapObject().graphs();
        if (graphs["default-graph-uri"]) d_graph = graphs["default-graph-uri"].as<string>().c_str();
        else d_graph = NULL;
        if (graphs["named-graph-uri"]) n_graph = graphs["named-graph-uri"].as<string>().c_str();
        else n_graph = NULL;
    }

    this->subscriptions.push_back(sepa_subscribe(sparql.c_str(),
                                                 alias.c_str(),
                                                 d_graph,
                                                 n_graph,
                                                 handler,
                                                 channel));
}

void SEPA::unsubscribe(std::string alias) {
    for (auto& sub: this->subscriptions) {
        auto sub_alias = std::string(sub->alias);
        if (sub_alias == alias) {
            sepa_unsubscribe(sub);
            break;
        }
    }
}

psClient SEPA::getpSecureClient() {
    return &(this->secure_client);
}
