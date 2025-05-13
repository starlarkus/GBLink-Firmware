#include <zephyr/kernel.h>

typedef uint16_t (*TransiveHandler)(uint16_t rx);

typedef void (*TransiveDoneHandler)(void);

void link_setTransiveCallback(TransiveHandler cb);

void link_setTransiveDoneCallback(TransiveDoneHandler cb);