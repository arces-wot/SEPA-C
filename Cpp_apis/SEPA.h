#ifndef SEPA_H
#define SEPA_H

#include <sstream>
#include "YSAPObject.h"
#include "../Sepa-Api-C/sepa_utils.h"
#include "../Sepa-Api-C/query.h"
#include "../Sepa-Api-C/update.h"
#include "../Sepa-Api-C/subscription.h"

#ifdef WIN32
#include <rpc.h>
#else
#include <uuid/uuid.h>
#endif

namespace sepa {
    class SEPA {
        public:
            SEPA(std::string client_id, ysap::YSAPObject sap_object);
            SEPA(ysap::YSAPObject sap_object);
            SEPA(std::string client_id);
            SEPA();
            virtual ~SEPA();

            void setClientId(std::string client_id);
            const std::string getClientId();

            void setSapObject(ysap::YSAPObject sap_object);
            ysap::YSAPObject getSapObject();

            const long query_unsecure(std::string identifier,
                                      YAML::Node bindings,
                                      std::ostream *destination,
                                      std::string host = "");
            const long sparql_query_unsecure(std::string sparql,
                                             std::ostream *destination,
                                             std::string host = "");

            const long query_secure(std::string identifier,
                                    YAML::Node bindings,
                                    std::ostream *destination,
                                    std::string host = "",
                                    std::string registration_url = "",
                                    std::string token_url = "");
            const long sparql_query_secure(std::string sparql,
                                           std::ostream *destination,
                                           std::string host = "",
                                           std::string registration_url = "",
                                           std::string token_url = "");

            const void storeSecureClient(std::string filepath);
            const void setSecureClient(std::string filepath);

            const long update_unsecure(std::string identifier,
                                       YAML::Node bindings,
                                       std::ostream *destination,
                                       std::string host = "");
            const long sparql_update_unsecure(std::string sparql,
                                              std::ostream *destination,
                                              std::string host = "");
            const long update_secure(std::string identifier,
                                     YAML::Node bindings,
                                     std::ostream *destination,
                                     std::string host = "",
                                     std::string registration_url = "",
                                     std::string token_url = "");
            const long sparql_update_secure(std::string sparql,
                                            std::ostream *destination,
                                            std::string host = "",
                                            std::string registration_url = "",
                                            std::string token_url = "");

            void subscribe(std::string identifier,
                           YAML::Node bindings,
                           std::string alias,
                           SubscriptionHandler handler,
                           pSubscriptionChannel channel,
                           std::string default_graph = "",
                           std::string named_graph = "");
            void sparql_subscribe(std::string sparql,
                                  std::string alias,
                                  SubscriptionHandler handler,
                                  pSubscriptionChannel channel);
            void unsubscribe(std::string alias);
            psClient getpSecureClient();

        protected:

        private:
            std::string client_id;
            ysap::YSAPObject sap_object;
            sClient secure_client;
            std::vector<pSubscription> subscriptions;
            void default_handler(void *result);
    };

const std::string uuidGenerator();

}

#endif // SEPA_H
