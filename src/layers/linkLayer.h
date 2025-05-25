#include <zephyr/kernel.h>

typedef uint16_t (*TransiveHandler)(uint16_t rx, void* user_data);

typedef void (*TransiveDoneHandler)(void* user_data);

void link_setTransiveCallback(TransiveHandler cb, void* user_data);

void link_setTransiveDoneCallback(TransiveDoneHandler cb, void* user_data);