/*
 * YSAPObject.cpp
 *
 *  Created on: 1 mar 2019
 *      Author: fr4ncidir
 */

#include "YSAPObject.h"

using namespace ysap;

YSAPObject::YSAPObject(std::string path_to_file) {
    path_to_ysap = path_to_file;
    ysap = YAML::LoadFile(path_to_file);
}

YSAPObject::YSAPObject() {
    path_to_ysap = "";
}

YSAPObject::~YSAPObject() {}

const YAML::Node YSAPObject::getYsap() {
    assert(!this->path_to_ysap.empty());
    return this->clone().ysap;
}

const YAML::Node YSAPObject::explore(std::vector<std::string> path) {
    YAML::Node currentNode = this->getYsap();
    for (auto& step: path) {
        currentNode = currentNode[step];
    }
    return currentNode;
}

const string YSAPObject::host() {
    assert(!this->path_to_ysap.empty());
    return this->ysap["host"].as<string>();
}

const YAML::Node YSAPObject::graphs() {
    assert(!this->path_to_ysap.empty());
    return this->ysap["graphs"];
}

const string YSAPObject::registration_url() {
    const char *path[] = {"oauth", "register"};
    return this->explore(vector<string>(path, end(path))).as<string>();
}

const string YSAPObject::tokenRequest_url() {
    const char *path[] = {"oauth", "tokenRequest"};
    return this->explore(vector<string>(path, end(path))).as<string>();
}

YAML::Node YSAPObject::sparql11protocol(vector<string> path) {
    auto g_path = vector<string>(path);
    g_path.insert(g_path.begin(), "sparql11protocol");
    return this->explore(g_path);
}

YAML::Node YSAPObject::sparql11seprotocol(vector<string> path) {
    auto g_path = vector<string>(path);
    g_path.insert(g_path.begin(), "sparql11seprotocol");
    return this->explore(g_path);
}

const string YSAPObject::query_url() {
    assert(!this->path_to_ysap.empty());
    auto protocol_path = vector<string>(1, "protocol");
    auto port_path = vector<string>(1, "port");
    const char *url_path[] = {"query", "path"};
    auto path_path = vector<string>(url_path, end(url_path));
    auto url = UriTools(this->sparql11protocol(protocol_path).as<string>(),
                        this->host(),
                        this->sparql11protocol(port_path).as<string>(),
                        this->sparql11protocol(path_path).as<string>());
    return url.getUri();
}

const string YSAPObject::update_url() {
    assert(!this->path_to_ysap.empty());
    auto protocol_path = vector<string>(1, "protocol");
    auto port_path = vector<string>(1, "port");
    const char *url_path[] = {"update", "path"};
    auto path_path = vector<string>(url_path, end(url_path));

    auto url = UriTools(this->sparql11protocol(protocol_path).as<string>(),
                        this->host(),
                        this->sparql11protocol(port_path).as<string>(),
                        this->sparql11protocol(path_path).as<string>());
    return url.getUri();
}

const string YSAPObject::subscribe_url() {
    assert(!this->path_to_ysap.empty());
    auto protocol_path = vector<string>(1, "protocol");
    auto use_protocol = this->sparql11seprotocol(protocol_path).as<std::string>();

    const char *url_port_path[] = {"availableProtocols", "port"};
    auto port_path = vector<string>(url_port_path, end(url_port_path));
    port_path.insert(port_path.begin()+1, use_protocol);

    const char *url_path_path[] = {"availableProtocols", "path"};
    auto path_path = vector<string>(url_path_path, end(url_path_path));
    path_path.insert(path_path.begin()+1, use_protocol);

    auto url = UriTools(use_protocol,
                        this->host(),
                        this->sparql11seprotocol(port_path).as<string>(),
                        this->sparql11seprotocol(path_path).as<string>());
    return url.getUri();
}

YSAPObject YSAPObject::clone() {
    assert(!this->path_to_ysap.empty());
    auto var = YSAPObject(path_to_ysap);
    return var;
}

const YAML::Node YSAPObject::updates() {
    const char *path[] = {"updates"};
    return this->explore(vector<string>(path, end(path)));
}

const YAML::Node YSAPObject::queries() {
    const char *path[] = {"queries"};
    return this->explore(vector<string>(path, end(path)));
}

const string YSAPObject::sparqlBuilder(YAML::Node unbound_sparql, YAML::Node bindings, vector<string> namespaces) {
    assert(!this->path_to_ysap.empty());
    ostringstream sparql;
    for (const auto& ns: namespaces) {
        sparql << ns << " ";
    }
    sparql << unbound_sparql.as<string>();

    auto sparql_str = sparql.str();
    for (YAML::const_iterator it=bindings.begin(); it!=bindings.end(); it++) {
        ostringstream binding_key;
        binding_key << "\\?" << it->first.as<string>();
        auto rgx = regex(binding_key.str());
        if (!it->second["value"].IsNull()) {
            if ((it->second["type"].as<string>() == "literal") && (it->second["value"].as<string>() != "UNDEF")) {
                ostringstream sparql_literal;
                sparql_literal << "'" << it->second["value"] << "'";
                sparql_str = regex_replace(sparql_str, rgx, sparql_literal.str());
                //boost::replace_all(sparql_str, binding_key.str(), sparql_literal.str());
            }
            else {
                auto uri = UriTools(it->second["value"].as<string>());
                sparql_str = regex_replace(sparql_str, rgx, uri.getFormattedUri());
                //boost::replace_all(sparql_str, binding_key.str(), uri.getFormattedUri());
            }
        }
    }
    return sparql_str;
}

const YAML::Node YSAPObject::getNamespaces() {
    const char *path[] = {"namespaces"};
    return this->explore(vector<string>(path, end(path)));
}

const vector<string> YSAPObject::getSparqlNamespaces() {
    auto my_namespaces = this->getNamespaces();
    auto sparql_namespaces = vector<string>();
    for (YAML::const_iterator it=my_namespaces.begin(); it!=my_namespaces.end(); it++) {
        ostringstream prefix_string_builder;
        prefix_string_builder << "PREFIX " << it->first.as<string>() << ": <" << it->second.as<string>() << ">";
        sparql_namespaces.push_back(prefix_string_builder.str());
    }
    return sparql_namespaces;
}

bool YSAPObject::checkBindings(YAML::Node current, YAML::Node expected) {
    assert(!this->path_to_ysap.empty());
    assert(current);
    assert(expected);
    auto current_keys = ysap::keys(current);
    string *current_keys_array = &current_keys[0];
    auto expected_keys = ysap::keys(expected);
    string *expected_keys_array = &expected_keys[0];
    auto key_difference = vector<string>(expected_keys.size());

    auto it = set_difference(expected_keys_array,
                             expected_keys_array+expected_keys.size(),
                             current_keys_array,
                             current_keys_array+current_keys.size(),
                             key_difference.begin());
    key_difference.resize(it-key_difference.begin());

    if (key_difference.empty()) return true;

    ostringstream exception_string;
    exception_string << key_difference[0] << " is a required forcedBinding";
    mylog::error(exception_string.str());
    return false;
}

const string YSAPObject::getSparql(YAML::Node sparql_set, string identifier, YAML::Node forced_bindings, bool bind_check) {
    assert(!this->path_to_ysap.empty());
    YAML::Node bindings;
    if (sparql_set[identifier]["forcedBindings"]) {
        bindings = sparql_set[identifier]["forcedBindings"];
        if (bind_check) assert(checkBindings(forced_bindings, bindings));
        if (forced_bindings) {
            auto fb_keys = ysap::keys(forced_bindings);
            string *fb_keys_array = &fb_keys[0];
            auto b_keys = ysap::keys(bindings);
            string *b_keys_array = &b_keys[0];
            auto key_intersection = vector<string>(min(fb_keys.size(), b_keys.size()));
            auto it = set_intersection(fb_keys_array,
                                       fb_keys_array+fb_keys.size(),
                                       b_keys_array,
                                       b_keys_array+b_keys.size(),
                                       key_intersection.begin());
            key_intersection.resize(it-key_intersection.begin());
            for (const auto& key: key_intersection) {
                bindings[key]["value"] = forced_bindings[key];
            }
        }
    }
    else {
        bindings = YAML::Node();
    }
    return this->sparqlBuilder(sparql_set[identifier]["sparql"], bindings, this->getSparqlNamespaces());
}

const std::string YSAPObject::getQuery(std::string identifier, YAML::Node bindings, bool bind_check) {
    return this->getSparql(this->queries(), identifier, bindings, bind_check);
}

const std::string YSAPObject::getUpdate(std::string identifier, YAML::Node bindings, bool bind_check) {
    return this->getSparql(this->updates(), identifier, bindings, bind_check);
}

bool YSAPObject::isEmpty() {
    return this->path_to_ysap.empty();
}

vector<string> ysap::keys(YAML::Node node) {
    auto keyList = vector<string>();
    for (YAML::const_iterator it=node.begin(); it!=node.end(); it++) {
        keyList.push_back(it->first.as<string>());
    }
    return keyList;
}
