#pragma once

#include <string>

using namespace std;

class UrlNormalizer {
public:
    // Normalizes user input into a complete, valid HTTP/HTTPS URL
    static string normalize(const string& rawInput);

    // Checks if the given hostname or IP corresponds to a local network / LAN address
    static bool isLocalAddress(const string& host);

    // Extracts the base host/domain from a URL
    static string extractHost(const string& url);
};
