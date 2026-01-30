#include <zephyr/kernel.h>
#include "TransiveStruct.hpp"

TransiveStruct usbLinkCommand();

void usbLink_receiveHandler(std::span<const uint8_t> data, void*);