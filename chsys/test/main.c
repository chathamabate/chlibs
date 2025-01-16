
#include <unistd.h>
#include "chsys/sys.h"
#include "sys.h"
#include "chsys/wrappers.h"

// We won't have UNITY tests here.
// Just some general tests that multiprocessing is working as
// expected.

int main(void) {
    run_sys_tests();
}
