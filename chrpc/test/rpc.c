
#include "chrpc/channel.h"
#include "chrpc/channel_local.h"
#include "chrpc/channel_local2.h"
#include "chrpc/serial_type.h"
#include "unity/unity.h"

#include "./rpc.h"
#include "chrpc/rpc.h"

static void test_chrpc_endpoint_create_and_delete(void) {
    chrpc_endpoint_t *ep;

    ep  = new_chrpc_endpoint_va(
        "login",
        NULL,

        CHRPC_STRING_T,
        new_chrpc_array_type(CHRPC_BYTE_T)
    );

    delete_chrpc_endpoint(ep);

    ep = new_chrpc_endpoint_va(
        "login",
        NULL,

        CHRPC_STRING_T,
        NULL
    );

    delete_chrpc_endpoint(ep);

    ep = new_chrpc_endpoint_va(
        "login",
        NULL,

        NULL,
        NULL
    );

    delete_chrpc_endpoint(ep);
}

static void test_chrpc_endpoint_set_creation_failure(void) {
    chrpc_endpoint_t *ep = new_chrpc_endpoint_argless(
        "login",
        NULL,
        NULL
    );

    chrpc_endpoint_set_t *ep_set = new_chrpc_endpoint_set_va(
        ep, ep
    );

    TEST_ASSERT_NULL(ep_set);

    delete_chrpc_endpoint(ep);
}

static void test_chrpc_endpoint_lookup(void) {
    // Using NULL function values here, there unused by this test.
    chrpc_endpoint_set_t *ep_set = new_chrpc_endpoint_set_va(
        new_chrpc_endpoint_va(
            "login",
            NULL,

            CHRPC_UINT64_T, // Returns an ID.

            CHRPC_STRING_T, // Username
            CHRPC_STRING_T  // Password
        ),
        new_chrpc_endpoint_va(
            "get_messages",
            NULL,
            
            new_chrpc_array_type(CHRPC_STRING_T), // List of messages.
                                                  
            CHRPC_UINT64_T // ID
        )
    );

    TEST_ASSERT_NULL(chrpc_endpoint_set_lookup(ep_set, "bobby"));

    const chrpc_endpoint_t *ep = chrpc_endpoint_set_lookup(ep_set, "login");
    TEST_ASSERT_NOT_NULL(ep);
    TEST_ASSERT_TRUE(chrpc_type_equals(CHRPC_UINT64_T, ep->ret));

    delete_chrpc_endpoint_set(ep_set);
}

// Now for the real RPC tests.

static void new_local_client(chrpc_server_t *server, const channel_local_config_t *cfg, 
        channel_local2_core_t **core, chrpc_client_t **client) {
    channel_status_t chn_status;


    // Now for the client addition stuff...
}


void chrpc_rpc_tests(void) {
    RUN_TEST(test_chrpc_endpoint_create_and_delete);
    RUN_TEST(test_chrpc_endpoint_set_creation_failure);
    RUN_TEST(test_chrpc_endpoint_lookup);
}
