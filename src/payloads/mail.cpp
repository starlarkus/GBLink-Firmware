#include "mail.hpp"

#include <bit>

static constexpr Mail g_emptyMail[16];

std::span<const std::byte> getEmptyMailPayload()
{
    return std::as_bytes(std::span(g_emptyMail));
}