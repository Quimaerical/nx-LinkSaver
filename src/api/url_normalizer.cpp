#include "api/url_normalizer.hpp"

#include <algorithm>
#include <cctype>
#include <regex>

using namespace std;

static string trim(const string& s) {
    auto start = s.begin();
    while (start != s.end() && isspace(static_cast<unsigned char>(*start))) {
        start++;
    }
    auto end = s.end();
    do {
        end--;
    } while (distance(start, end) > 0 && isspace(static_cast<unsigned char>(*end)));
    return string(start, end + 1);
}

string UrlNormalizer::normalize(const string& rawInput) {
    string cleaned = trim(rawInput);
    if (cleaned.empty()) {
        return "";
    }

    // Remove any trailing slashes
    while (cleaned.length() > 1 && cleaned.back() == '/') {
        cleaned.pop_back();
    }

    bool hasScheme = (cleaned.find("http://") == 0 || cleaned.find("https://") == 0);
    string scheme;
    string addressPart;

    if (hasScheme) {
        size_t schemeEnd = cleaned.find("://");
        scheme = cleaned.substr(0, schemeEnd);
        addressPart = cleaned.substr(schemeEnd + 3);
    } else {
        addressPart = cleaned;
    }

    // Remove trailing slash from addressPart if present
    while (!addressPart.empty() && addressPart.back() == '/') {
        addressPart.pop_back();
    }

    string host = extractHost(addressPart);
    bool local = isLocalAddress(host);

    if (scheme.empty()) {
        if (local) {
            scheme = "http";
        } else {
            scheme = "https";
        }
    }

    // Check if port is specified in addressPart
    bool hasPort = false;
    size_t colonPos = addressPart.find(':');
    if (colonPos != string::npos) {
        // Ensure it's not followed by a path slash
        size_t slashPos = addressPart.find('/', colonPos);
        if (slashPos == string::npos) {
            hasPort = true;
        } else if (slashPos > colonPos) {
            hasPort = true;
        }
    }

    // If it's a local address without explicit port and without custom path, append default port 8080
    if (local && !hasPort && addressPart.find('/') == string::npos) {
        addressPart += ":8080";
    }

    return scheme + "://" + addressPart;
}

bool UrlNormalizer::isLocalAddress(const string& host) {
    if (host == "localhost" || host == "127.0.0.1") {
        return true;
    }

    // Regex for private IPv4 addresses:
    // 10.0.0.0/8
    // 172.16.0.0/12
    // 192.168.0.0/16
    regex ipPattern(R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$)");
    smatch match;
    if (regex_match(host, match, ipPattern)) {
        int o1 = stoi(match[1]);
        int o2 = stoi(match[2]);

        if (o1 == 10) return true;
        if (o1 == 192 && o2 == 168) return true;
        if (o1 == 172 && (o2 >= 16 && o2 <= 31)) return true;
        if (o1 == 127) return true;
    }

    return false;
}

string UrlNormalizer::extractHost(const string& url) {
    string s = url;
    size_t schemePos = s.find("://");
    if (schemePos != string::npos) {
        s = s.substr(schemePos + 3);
    }

    size_t slashPos = s.find('/');
    if (slashPos != string::npos) {
        s = s.substr(0, slashPos);
    }

    size_t colonPos = s.find(':');
    if (colonPos != string::npos) {
        s = s.substr(0, colonPos);
    }

    return s;
}
