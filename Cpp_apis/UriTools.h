#ifndef URITOOLS_H
#define URITOOLS_H

#include <sstream>
#include <uriparser/Uri.h>
#include <cassert>
#include <string>
#include "mylogger.h"

using namespace std;

class UriTools {
    public:
        UriTools(string schema, string host, string port, string path);
        UriTools(string uri);
        UriTools(const char *uri);
        virtual ~UriTools();
        const string getSchema();
        const string getHost();
        const string getPort();
        const string getPath();
        const string getUri();
        const string getFormattedUri();

    protected:

    private:
        bool status;
        string uri_string;
        UriUriA parsedUri;
        void setParsedUri(const char *uri_string);
};

#endif // URITOOLS_H
