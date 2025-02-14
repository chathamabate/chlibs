

#ifndef CHHTTP_REQUEST_H
#define CHHTTP_REQUEST_H

#include "chutil/string.h"
#include "chutil/map.h"

// A header map is really just a map<string *, string *>
typedef hash_map_t header_map_t;

static inline header_map_t *new_header_map() {
    return new_hash_map(sizeof(string_t *), sizeof(string_t *), (hash_map_hash_ft)s_indirect_hash, (hash_map_key_eq_ft)s_indirect_equals);
}

void delete_header_map(header_map_t *hm);

void header_map_put(const char *k, const char *v);

// NOTE: Right now, we will not support sending chunked responses.
// We will be able to receive them, but that's it.

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

    header_map_t *headers; 

    // NOTE: We don't support streamed requests.
    size_t body_length;
    void *body;
} http_req_t;

// The http request will take OWNERSHIP of the headers map, and the body.
// The http request DOES NOT take OWNERSHIP of the target string, it will be copied.
http_req_t *new_http_request_verbose(http_method_t m, const char *target, header_map_t *headers, void *body, size_t body_length);

static inline http_req_t *new_http_request_bodied(http_method_t m, const char *target, void *body, size_t body_length) {
    return new_http_request_verbose(m, target, new_header_map(), body, body_length);
}

static inline http_req_t *new_http_request(http_method_t m, const char *target) {
    return new_http_request_bodied(m, target, NULL, 0);
}

void delete_http_request(http_req_t *req);


#endif
