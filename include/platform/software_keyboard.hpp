#pragma once

#include <string>

using namespace std;

class SoftwareKeyboard {
public:
    // Opens Horizon OS on-screen virtual keyboard (or desktop fallback) and returns entered text
    static string prompt(const string& guideText, const string& initialText = "", size_t maxLen = 256);
};
