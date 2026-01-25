#include <zephyr/kernel.h>
#include "TransiveStruct.hpp"
#include <optional>

TransiveStruct usbLinkCommand();

void usbLink_receiveHandler(std::span<const uint8_t> data, void*);