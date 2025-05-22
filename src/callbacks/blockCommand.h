#include <zephyr/kernel.h>

void blockCommandSetup(const void* src, uint16_t size, uint16_t blockMaxSize);
bool blockCommandChunk();

uint16_t blockCommand_cb();