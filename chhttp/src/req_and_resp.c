
#include <stdlib.h>
#include "chhttp/req_and_resp.h"

const char *http_method_as_literal(http_method_t m) {
    switch (m) {
        case HTTP_GET:
            return "GET";
        case HTTP_HEAD:
            return "HEAD";
        case HTTP_OPTIONS:
            return "OPTIONS";
        case HTTP_PUT:
            return "PUT";
        case HTTP_DELETE:
            return "DELETE";
        case HTTP_POST:
            return "POST";
        case HTTP_PATCH:
            return "PATCH";
        case HTTP_TRACE:
            return "TRACE";
        case HTTP_CONNECT:
            return "CONNECT";
        default:
            return NULL;
    }
}
