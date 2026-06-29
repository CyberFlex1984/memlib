#pragma once

#include "memlib/types.hpp"
#include <algorithm>
#include <optional>
#include <ranges>
#include <sstream>
#include <vector>


namespace memlib {
    class Pattern {
        std::vector<std::optional<memlib::byte_t>> bytes;
    public:
        Pattern() = default;

        explicit Pattern(const std::string& signature) {
            std::istringstream iss(signature);
            std::string token;
            while(iss >> token) {
                if(token == "?" || token == "??") {
                    bytes.push_back(std::nullopt);
                }
                else{
                    bytes.push_back(static_cast<byte_t>(std::stoi(token, nullptr, 16)));
                }
            }
        }
        Pattern(const std::vector<std::optional<memlib::byte_t>> bytes) : bytes(bytes) {}

        bool matches(const memlib::byte_t* data, size_t offset) const {
            return std::ranges::all_of(
                std::views::iota(size_t{0}, bytes.size()),
                [&](size_t i){
                    return !bytes[i].has_value() || data[offset + i] == bytes[i].value();
                }
            );
        }
        bool matches(const memlib::byte_array& data, size_t offset) const {
            return matches(data.data(), offset);
        }

        memlib::size_t size() const { return bytes.size(); }
        bool empty() const { return bytes.empty(); }
        const auto& get_bytes() const { return bytes; }
    };
}