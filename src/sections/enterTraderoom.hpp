#include <span>

enum class ConnectionStatus
{
    open,
    closed
};

class EnterTraderoom
{
    ConnectionStatus process(std::span<const uint8_t> command);
};