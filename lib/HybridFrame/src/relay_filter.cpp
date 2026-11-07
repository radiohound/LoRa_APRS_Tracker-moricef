#include "relay_filter.h"

namespace RELAY_FILTER {

    static bool isDigit(char c) { return c >= '0' && c <= '9'; }
    static bool isAlpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
    static bool isAlphaNumeric(char c) { return isDigit(c) || isAlpha(c); }

    bool callsignIsValid(const std::string& callsign) {
        if (callsign == "WLNK-1") return true;
        int totalCallsignLength = (int)callsign.length();
        if (totalCallsignLength < 4) return false;

        int hyphenIndex         = (int)callsign.find('-');
        if (hyphenIndex == (int)std::string::npos) hyphenIndex = -1;
        int baseCallsignLength  = (hyphenIndex > 0) ? hyphenIndex : totalCallsignLength;

        if (hyphenIndex > 0) {    // SSID Validation
            if (hyphenIndex < 4) return false;
            int ssidStart   = hyphenIndex + 1;
            int ssidLength  = totalCallsignLength - ssidStart;
            if (ssidLength == 0 || ssidLength > 2) return false;
            if (callsign.find('-', (size_t)ssidStart) != std::string::npos) return false;
            if (ssidLength == 2 && callsign[(size_t)ssidStart] == '0') return false;
            for (int i = ssidStart; i < totalCallsignLength; i++) {
                if (!isDigit(callsign[(size_t)i])) return false;
            }
        }

        if (baseCallsignLength < 4 || baseCallsignLength > 6) return false;

        bool padded = false;    // callsigns with 4 characters like A0AA are padded to 5 for the analysis: A0AA --> _A0AA
        if (baseCallsignLength == 4 && isAlpha(callsign[0]) && isDigit(callsign[1]) && isAlpha(callsign[2]) && isAlpha(callsign[3])) padded = true;
        char c0, c1, c2, c3;
        if (padded) {
            c0 = ' ';
            c1 = callsign[0];
            c2 = callsign[1];
            c3 = callsign[2];
        } else {
            c0 = callsign[0];
            c1 = callsign[1];
            c2 = callsign[2];
            c3 = callsign[3];
        }
        if (!isDigit(c2) || !isAlpha(c3)) {
            if (c0 != 'R' && !isDigit(c1) && !isAlpha(c2)) return false;
        }

        bool isValid =
            ((isAlphaNumeric(c0) || c0 == ' ') && isAlpha(c1)) ||
            (isAlpha(c0) && isDigit(c1)) ||
            (c0 == 'R' && baseCallsignLength == 6 && isDigit(c1) && isAlpha(c2) && isAlpha(c3) && isAlpha(callsign[4]));
        if (!isValid) return false;

        if (baseCallsignLength > 4) {
            for (int i = 4; i < baseCallsignLength; i++) {
                if (!isAlpha(callsign[(size_t)i])) return false;
            }
        }
        return true;
    }

    bool refused(const std::string& front) {
        const size_t gt = front.find('>');
        if (gt == std::string::npos || gt == 0) return true;
        const size_t colon = front.find(':');
        const std::string head = front.substr(0, colon == std::string::npos ? front.size() : colon);
        if (head.find("NOGATE") != std::string::npos) return true;                // the iGate's digipeater does not repeat it
        return !callsignIsValid(front.substr(0, gt));
    }

}
