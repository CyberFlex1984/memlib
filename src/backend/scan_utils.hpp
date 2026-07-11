#pragma once

#include "memlib/pattern.hpp"
#include "memlib/types.hpp"
#include <algorithm>
#include <iterator>
#include <ranges>
#include <vector>

namespace memlib {
    inline Result<std::vector<address_t>> scan_buffer(
        const Pattern& pattern,
        const byte_array& buffer,
        address_t base_address
    ) {
        size_t size = buffer.size();
        size_t pattern_size = pattern.size();

        if(size < pattern_size){
            return std::vector<address_t>{};
        }

        auto positions = std::views::iota(size_t{0}, size - pattern_size + 1)
            | std::views::filter([&](size_t offset){
                return pattern.matches(buffer.data(), offset);
            })
            | std::views::transform([&](size_t offset){
                return base_address + offset;
            });

        std::vector<address_t> results{};
        std::ranges::copy(positions, std::back_inserter(results));

        return results;
    }

    template<typename Backend>
    Result<std::vector<address_t>> scan_region(
        Backend& backend,
        const Pattern& pattern,
        address_t start,
        address_t end
    ) {
        size_t size = end - start;
        byte_array buffer(size);

        auto read_result = backend.read_bytes(start, buffer.data(), size);
        if (!read_result) {
            return std::unexpected(read_result.error());
        }

        return scan_buffer(pattern, buffer, start);
    }
}
