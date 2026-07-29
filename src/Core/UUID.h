
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

#include "Core.h"

namespace lgt {
class LGT_API UUID {
public:
    UUID() { generate(); }
    explicit UUID(const std::string& str)
        : value(str) {}

    // Returns string representation
    const std::string& str() const { return value; }

    // Operators
    bool operator==(const UUID& other) const { return value == other.value; }
    bool operator!=(const UUID& other) const { return !(*this == other); }
    bool operator<(const UUID& other) const { return value < other.value; }

private:
    std::string value;

    void generate() {
        static thread_local std::random_device                      rd;
        static thread_local std::mt19937_64                         gen(rd());
        static thread_local std::uniform_int_distribution<uint64_t> dis;

        std::array<uint8_t, 16> bytes;
        uint64_t                p1 = dis(gen);
        uint64_t                p2 = dis(gen);

        for (int i = 0; i < 8; i++)
            bytes[i] = static_cast<uint8_t>((p1 >> (i * 8)) & 0xFF);
        for (int i = 0; i < 8; i++)
            bytes[i + 8] = static_cast<uint8_t>((p2 >> (i * 8)) & 0xFF);

        // Apply RFC 4122 variant and version
        bytes[6] = (bytes[6] & 0x0F) | 0x40; // Version 4 (random)
        bytes[8] = (bytes[8] & 0x3F) | 0x80; // Variant 1 (RFC)

        // Convert to string
        std::ostringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < bytes.size(); ++i) {
            ss << std::setw(2) << static_cast<int>(bytes[i]);
            if (i == 3 || i == 5 || i == 7 || i == 9)
                ss << "-";
        }
        value = ss.str();
    }
};
} // namespace lgt

namespace std {
    template<>
    struct hash<lgt::UUID> {
        std::size_t operator()(const lgt::UUID& uuid) const {
            return std::hash<std::string>()(uuid.str());
        }
    };
}
