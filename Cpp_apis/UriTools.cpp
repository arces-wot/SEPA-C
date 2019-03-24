#include "UriTools.h"


void UriTools::setParsedUri(const char *uri_string) {
    const char *errorPos;
    this->status = true;
    assert(uri_string != nullptr);
    if (uriParseSingleUriA(&(this->parsedUri), uri_string, &errorPos) != URI_SUCCESS) {
        ostringstream error_msg;
        error_msg << "Error while parsing " << string(uri_string) << endl;
        mylog::error(error_msg.str());
        this->status = false;
    }
}

UriTools::UriTools(string schema, string host, string port, string path) {
    ostringstream uri;
    uri << schema << "://" << host;
    if (!port.empty()) {
       uri << ":" << port;
    }
    uri << path;
    this->uri_string = uri.str();
    setParsedUri(uri_string.c_str());
}

UriTools::UriTools(string uri) {
    this->uri_string = uri;
    setParsedUri(uri.c_str());
}

UriTools::UriTools(const char *uri) {
    this->uri_string = string(uri);
    setParsedUri(uri);
}

UriTools::~UriTools() {
    uriFreeUriMembersA(&(this->parsedUri));
}

const string UriTools::getSchema() {
    string protocol;
    if (this->status) {
        protocol.assign(this->parsedUri.scheme.first,
                        this->parsedUri.scheme.afterLast - this->parsedUri.scheme.first);
    }
    else {
        protocol = this->uri_string;
    }
    return protocol;
}

const string UriTools::getHost() {
    string host;
    if (this->status) {
        host.assign(this->parsedUri.hostText.first,
                    this->parsedUri.hostText.afterLast - this->parsedUri.hostText.first);
    }
    else {
        host = this->uri_string;
    }
    return host;
}

const string UriTools::getPort() {
    string port;
    if (this->status) {
        port.assign(this->parsedUri.portText.first,
                    this->parsedUri.portText.afterLast - this->parsedUri.portText.first);
    }
    else {
        port = this->uri_string;
    }
    return port;
}

const string UriTools::getPath() {
    string port;
    if (this->status) {
        port.assign(this->parsedUri.pathHead->text.first,
                    this->parsedUri.pathTail->text.afterLast - this->parsedUri.pathHead->text.first);
    }
    else {
        port = this->uri_string;
    }
    return port;
}

const string UriTools::getUri() {
    return this->uri_string;
}

const string UriTools::getFormattedUri() {
    auto full_uri = this->getUri();
    if ((!this->status) || (full_uri == "UNDEF") || ((!this->getSchema().empty()) && (this->getHost().empty())  && (this->getPort().empty()) && (!this->getPath().empty()))) {
        return full_uri;
    }
    ostringstream formatted_uri;
    formatted_uri << "<" << full_uri << ">";
    return formatted_uri.str();
}
