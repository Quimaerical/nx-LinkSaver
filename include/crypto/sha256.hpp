#pragma once

#include <string>

using namespace std;

class Sha256 {
public:
    // Computes SHA-256 hex string of a given file path
    static string calculateFileHash(const string& filePath);

    // Computes SHA-256 hex string of memory buffer
    static string calculateStringHash(const string& data);
};
