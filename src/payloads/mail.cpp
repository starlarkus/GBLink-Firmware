#include "mail.hpp"

static std::array<uint8_t, 220> g_array = []{
    std::array<uint8_t, 220> a{};
    a.fill(0xFF);
    return a;
}();

std::span<const std::byte> getEmptyMailPayload()
{
    return std::as_bytes(std::span(g_array));
}