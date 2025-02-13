
#ifndef CHTEST_MESSAGE_H
#define CHTEST_MESSAGE_H

typedef struct _chtest_message_t {
    // A comparison of two values??
    // Or a log message as a string??
    // Nah, no timestamps for messages??
    // Actually, timestamps since beginning of test wouldn't be so bad.
    // The running of the RPC wouldn't be all that great, but whatevs??
    // Always send, that way we can read output even in case of fatal error.
} chtest_message_t;
// A message is something that ultimately will show up in the 
// log.json file.
// I'm tired... ugh...
// I'd rather just deal with the message type tbh...

#endif
