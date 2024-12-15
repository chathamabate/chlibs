
#ifndef CHUTIL_MISC_H
#define CHUTIL_MISC_H

#define NUM_ARGS(arg_type,...) (sizeof((arg_type[]){__VA_ARGS__}) / sizeof(arg_type))

#endif

