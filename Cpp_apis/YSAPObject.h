/*
 * YSAPObject.h
 *
 *  Created on: 1 mar 2019
 *      Author: fr4ncidir
 */

#ifndef YSAPOBJECT_H_
#define YSAPOBJECT_H_

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <yaml-cpp/yaml.h>
#include <regex>

#include "UriTools.h"
#include "mylogger.h"

namespace ysap {

    class YSAPObject {
        public:
            YSAPObject(std::string path_to_file);
            YSAPObject();
            virtual ~YSAPObject();
            const YAML::Node getYsap();
            const YAML::Node explore(std::vector<std::string> path);
            const std::string host();
            const YAML::Node graphs();
            const std::string registration_url();
            const std::string tokenRequest_url();
            const std::string query_url();
            const std::string update_url();
            const std::string subscribe_url();
            const YAML::Node updates();
            const YAML::Node queries();
            const YAML::Node getNamespaces();
            const std::vector<std::string> getSparqlNamespaces();
            YSAPObject clone();
            const std::string getQuery(std::string identifier, YAML::Node bindings, bool bind_check);
            const std::string getUpdate(std::string identifier, YAML::Node bindings, bool bind_check);
            const std::string getSparql(YAML::Node sparql_set, std::string identifier, YAML::Node bindings, bool bind_check);
            bool isEmpty();

        protected:

        private:
            std::string path_to_ysap;
            YAML::Node ysap;
            YAML::Node sparql11protocol(std::vector<std::string> path);
            YAML::Node sparql11seprotocol(std::vector<std::string> path);
            const std::string sparqlBuilder(YAML::Node unbound_sparql, YAML::Node bindings, std::vector<std::string> namespaces);
            bool checkBindings(YAML::Node current, YAML::Node expected);
    };

    std::vector<std::string> keys(YAML::Node node);

}

#endif /* YSAPOBJECT_H_ */
