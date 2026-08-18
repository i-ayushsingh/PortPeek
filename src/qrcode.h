#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace QrCode {

struct QrMatrix {
    int size = 0;
    std::vector<uint8_t> modules; // 1 for black, 0 for white

    bool Get(int x, int y) const {
        if (x < 0 || x >= size || y < 0 || y >= size) return false;
        return modules[y * size + x] != 0;
    }

    void Set(int x, int y, bool val) {
        if (x >= 0 && x < size && y >= 0 && y < size) {
            modules[y * size + x] = val ? 1 : 0;
        }
    }
};

// Generates an ISO/IEC 18004 standard QR Code Model 2 using official Nayuki generator
QrMatrix Generate(const std::string& text);

} // namespace QrCode
