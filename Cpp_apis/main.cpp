#include <iostream>
#include <fstream>
#include <regex>
#include "YSAPObject.h"
#include "SEPA.h"

using namespace std;

sepa::SEPA engine = sepa::SEPA("CLIENTID");
sepa::SEPA engine_ysap = sepa::SEPA("ACLIENTID");
sepa::SEPA engine_secure_ysap = sepa::SEPA("SCLIENTID");

void how_to_parse_an_YSAPObject();
void how_to_make_unsecure_queries();
void how_to_make_secure_queries();
void how_to_make_unsecure_updates();
void how_to_make_secure_updates();
void how_to_make_unsecure_subscriptions();
void how_to_make_secure_subscriptions();

int main() {
    how_to_parse_an_YSAPObject();
    how_to_make_unsecure_queries();
    how_to_make_secure_queries();
    how_to_make_unsecure_updates();
    how_to_make_secure_updates();
    how_to_make_unsecure_subscriptions();
    how_to_make_secure_subscriptions();
    return 0;
}

void how_to_parse_an_YSAPObject() {
    auto ysapFile = ysap::YSAPObject("./example.ysap");
    YAML::Node forcedBindings_A;
    YAML::Node forcedBindings_B;
    forcedBindings_B["ds"] = "http://dataschema.example";

    cout << "\n------YSAPObject parsing-----\n" <<
            "Host: " << ysapFile.host() << "\n" <<
            "Query url: " << ysapFile.query_url() << "\n" <<
            "Update url: " << ysapFile.update_url() << "\n" <<
            "Subscribe url: " << ysapFile.subscribe_url() << "\n\n" <<
            "JSONLD_DS_CONSTRUCT query (unbound): " <<
                    ysapFile.getSparql(ysapFile.queries(),
                                       "JSONLD_DS_CONSTRUCT",
                                       forcedBindings_A,
                                       false) << "\n" <<
            "JSONLD_DS_CONSTRUCT query (bound): " <<
                ysapFile.getSparql(ysapFile.queries(),
                                   "JSONLD_DS_CONSTRUCT",
                                   forcedBindings_B,
                                   true) <<
            "\n------END--------------------\n" << endl;

    engine_ysap.setSapObject(ysapFile);

    engine_secure_ysap.setSapObject(ysap::YSAPObject("./example_secure.ysap"));;
}

void how_to_make_unsecure_queries() {
    ostringstream result;
    long r = engine.sparql_query_unsecure("select * where {?a ?b ?c}",
                                          &result,
                                          "http://localhost:8000/query");

    cout << "\n------Unsecure query---------\nResult code: " << r << ";\n";
    if (r == 200) cout << result.str();
    cout << "\n------END--------------------\n" << endl;

    ofstream fileout;
    fileout.open("./result.json");
    YAML::Node bindings;
    r = engine_ysap.query_unsecure("EVERYTHING",
                                   bindings,
                                   &fileout);
    fileout.close();
}


void how_to_make_secure_queries() {
    ostringstream result;
    long r = engine_secure_ysap.sparql_query_secure("select ?a ?b where {?a ?b ?c}", &result);
    cout << "\n------Secure query---------\nResult code: " << r << ";\n";
    if (r == 200) cout << result.str();
    cout << "\n------END--------------------\n" << endl;

    engine_secure_ysap.storeSecureClient("./sclient.jwt");
}


void how_to_make_unsecure_updates() {
    ostringstream r_in, r_out;
    long r = engine_ysap.sparql_update_unsecure("insert data {<http://pippo.org> <http://paperino.org> 'pluto'}", &r_in);

    cout << "\n------Unsecure update---------\nResult code: " << r << ";\n";
    if (r == 200) cout << r_in.str();
    cout << "\n------END--------------------\n" << endl;

    r = engine_ysap.update_unsecure("DELETE_EVERYTHING", YAML::Node(), &r_out);
    cout << "\n------Unsecure update---------\nResult code: " << r << ";\n";
    if (r == 200) cout << r_out.str();
    cout << "\n------END--------------------\n" << endl;
}


void how_to_make_secure_updates() {
    engine_secure_ysap.setSecureClient("./sclient.jwt");

    ostringstream r_in, r_out;
    long r = engine_secure_ysap.sparql_update_secure("insert data {<http://pippo.org> <http://paperino.org> 'topolino'}", &r_in);

    cout << "\n------Secure update---------\nResult code: " << r << ";\n";
    if (r == 200) cout << r_in.str();
    cout << "\n------END--------------------\n" << endl;

    r = engine_secure_ysap.update_secure("DELETE_EVERYTHING", YAML::Node(), &r_out);
    cout << "\n------Secure update---------\nResult code: " << r << ";\n";
    if (r == 200) cout << r_out.str();
    cout << "\n------END--------------------\n" << endl;
}


void how_to_make_unsecure_subscriptions() {
    SubscriptionChannel sc;
    long r;
    int i;
    r = new_unsecure_channel("ws://localhost:9000/subscribe",
                         2,
                         &sc);
    if (!r) {
        engine.sparql_subscribe("select * where {?a ?b ?c}",
                                    "cppSubscription",
                                    [](void * param) -> void* {
                                        cout << "\n\nResult of subscription in lambda (1) " << string((char *) param) << endl;
                                        return 0;
                                    },
                                    &sc);

        auto bd = YAML::Node();
        bd["name"] = "pippo";
        engine_ysap.subscribe("EVERYTHING_REDUCED",
                              bd,
                            "cppSubscription2",
                            [](void * param) -> void* {
                                cout << "\n\nResult of subscription in lambda (2) " << string((char *) param) << endl;
                                return 0;
                            },
                            &sc);
        cout << "Insert a number to continue..." << endl;
        cin >> i;
        engine.unsubscribe("cppSubscription");
        engine_ysap.unsubscribe("cppSubscription2");
        cout << "Insert a number to continue..." << endl;
        cin >> i;
        close_subscription_channel(&sc);
    }
}

void how_to_make_secure_subscriptions() {
    sepa::SEPA engine_s_ysap = sepa::SEPA("THISCLIENTID", ysap::YSAPObject("./example_secure.ysap"));
    engine_s_ysap.setSecureClient("./sclient.jwt");

    SubscriptionChannel sc;
    long r;
    int i;
    r = new_secure_channel(engine_s_ysap.getSapObject().subscribe_url().c_str(),
                         2,
                         "THISCLIENTID",
                         engine_s_ysap.getpSecureClient(),
                         &sc);
    if (!r) {
        engine_s_ysap.sparql_subscribe("select * where {?a ?b ?c}",
                                    "cppSubscription",
                                    [](void * param) -> void* {
                                        cout << "\n\nResult of subscription in lambda (1) " << string((char *) param) << endl;
                                        return 0;
                                    },
                                    &sc);

        auto bd = YAML::Node();
        bd["name"] = "pippo";
        engine_s_ysap.subscribe("EVERYTHING_REDUCED",
                              bd,
                            "cppSubscription2",
                            [](void * param) -> void* {
                                cout << "\n\nResult of subscription in lambda (2) " << string((char *) param) << endl;
                                return 0;
                            },
                            &sc);
        cout << "Insert a number to continue..." << endl;
        cin >> i;
        engine_s_ysap.unsubscribe("cppSubscription");
        engine_s_ysap.unsubscribe("cppSubscription2");
        cout << "Insert a number to continue..." << endl;
        cin >> i;
        close_subscription_channel(&sc);
    }
}
