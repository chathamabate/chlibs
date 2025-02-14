

#ifndef CHHTTP_REQUEST_H
#define CHHTTP_REQUEST_H

#include "chutil/string.h"
#include "chutil/map.h"

// Helper for creating a non-empty set of headers.
// Returns NULL on error. This errors out when trying to set an invalid header.
// For example, content length will be determined and written when necessary,
// you cannot specify it manually here.
//
// Call these functions by alternating keys and values as C-strings/literals.
hash_map_t *new_header_set_from_kvps(int dummy,...);
#define new_header_set_from_kvps(...) 

typedef enum _http_method_t {
    HTTP_GET = 0,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_POST,
    HTTP_PATCH,
    HTTP_TRACE,
    HTTP_CONNECT
} http_method_t;

const char *http_method_as_literal(http_method_t m);

typedef struct _http_req_t {
    http_method_t method;
    string_t *target;

    // NOTE: We don't store protocol as we will only support 
    // HTTP/1.1

    // Maps<string_t *, string_t *>
    hash_map_t headers; 

    // NOTE: We don't support streamed requests.
    size_t body_length;
    void *body;
} http_req_t;






#endif
