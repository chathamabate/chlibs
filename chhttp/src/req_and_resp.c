
#include <stdlib.h>
#include "chhttp/req_and_resp.h"
#include "chsys/mem.h"
#include "chutil/map.h"
#include "chutil/string.h"

void delete_header_map(header_map_t *hm) {
    key_val_pair_t kvp;
    hm_reset_iterator(hm);
    while ((kvp = hm_next_kvp(hm)) != HASH_MAP_EXHAUSTED) {
        string_t **s;

        s = (string_t **)kvp_key(hm, kvp);
        delete_string(*s);

        s = (string_t **)kvp_val(hm, kvp);
        delete_string(*s);
    }
    delete_hash_map(hm);
}

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

http_req_t *new_http_request_verbose(http_method_t m, const char *target, hash_map_t *headers, void *body, size_t body_length) {
    http_req_t *req = safe_malloc(sizeof(http_req_t)); 
    
    req->method = m;
    req->target = new_string_from_cstr(target);
    req->headers = headers;

    req->body_length = body_length;
    req->body = body;

    return req;
}

void delete_http_request(http_req_t *req) {
    delete_string(req->target);
    delete_header_map(req->headers);

    if (req->body) {
        safe_free(req->body);
    }

    safe_free(req);
}
