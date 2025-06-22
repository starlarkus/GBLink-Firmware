#include <zephyr/kernel.h>

enum LinkMode
{
    MASTER,
    SLAVE
};

typedef void (*ReceiveHandler)(uint16_t rx, void* userData);
typedef uint16_t (*TransmitHandler)(void* userData);
typedef void (*TransiveDoneHandler)(void* userData);

void link_setTransmitCallback(TransmitHandler cb, void* userData);

void link_setReceiveCallback(ReceiveHandler cb, void* userData);

void link_setTransiveDoneCallback(TransiveDoneHandler cb, void* userData);

enum LinkMode link_getMode();

void link_changeMode(enum LinkMode mode);