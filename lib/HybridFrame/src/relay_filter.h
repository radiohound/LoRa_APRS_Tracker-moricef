// What a station that relays a frame WITHOUT reading it (3C FF 03, a model or destination code it does not hold) still refuses to repeat: the same two rules the iGate's
// digipeater applies to every frame, NOGATE in the path and a sender that is not a valid callsign. Content nobody has read should be repeated no more freely than content that has.
// No hardware dependency (host-testable). callsignIsValid is the iGate's Utils::callsignIsValid on std::string.
#ifndef RELAY_FILTER_H_
#define RELAY_FILTER_H_

#include <string>

namespace RELAY_FILTER {
    bool callsignIsValid(const std::string& callsign);
    // front = "SRC>DST,PATH:" as HYBRID_FRAME::front gives it. True if the frame must not be relayed unread.
    bool refused(const std::string& front);
}

#endif
