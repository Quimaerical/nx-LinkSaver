#include "platform/software_keyboard.hpp"

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <iostream>
#include <vector>

using namespace std;

string SoftwareKeyboard::prompt(const string& guideText, const string& initialText, size_t maxLen) {
#ifdef __SWITCH__
    SwkbdConfig kbd;
    vector<char> outText(maxLen + 1, 0);

    Result rc = swkbdCreate(&kbd, 0);
    if (R_FAILED(rc)) {
        return initialText;
    }

    swkbdConfigMakePresetDefault(&kbd);
    swkbdConfigSetGuideText(&kbd, guideText.c_str());
    swkbdConfigSetInitialText(&kbd, initialText.c_str());
    swkbdConfigSetStringLenMax(&kbd, static_cast<u32>(maxLen));

    rc = swkbdShow(&kbd, outText.data(), outText.size());
    swkbdClose(&kbd);

    if (R_SUCCEEDED(rc)) {
        return string(outText.data());
    }
    return initialText;
#else
    // Desktop development fallback: if interactive or piped, return initialText if non-empty
    // In real Borealis desktop tests, this allows simulating input
    return initialText.empty() ? "" : initialText;
#endif
}
