#include "blockCommand.h"
#include "../link_defines.h"

#include <string.h>

#define MAX_CHUNK 14
#define BLOCK_SIZE_INDEX 1

static const void* g_src;
static uint16_t g_srcSize;
static uint16_t g_pos;

static uint16_t g_index = 0;
static bool g_initSend = false;
static uint16_t g_blockMaxSize = 0;

static uint16_t g_blockCommandInit[8] = {
    LINKCMD_INIT_BLOCK, 0x00, LINK_PLAYER_ID, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint16_t g_blockCommandContent[8] = {LINKCMD_CONT_BLOCK, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/**
 * @brief Chunks the block into parts for the callback to send
 * 
 * @return true Chunking is done
 * @return false More Chunking to do
 */
bool blockCommandChunk()
{
    if (g_blockMaxSize == 0) return true;

    if (!g_initSend) return false;

    memset((uint8_t*)g_blockCommandContent + 2, 0x00, MAX_CHUNK);

    uint16_t chunkSize = g_srcSize < MAX_CHUNK ? g_srcSize : MAX_CHUNK;
    uint16_t maxChunkSize = g_blockMaxSize < MAX_CHUNK ? g_blockMaxSize : MAX_CHUNK;

    if (g_blockMaxSize > 0)
    {
        memcpy((uint8_t*)g_blockCommandContent + 2, (uint8_t*)g_src + g_pos, chunkSize);
    }
    
    g_srcSize -= chunkSize;
    g_blockMaxSize -= maxChunkSize;
    g_pos += chunkSize;
    return (g_blockMaxSize == 0);
}

void blockCommandSetup(const void* src, uint16_t size, uint16_t blockMaxSize)
{
    g_src = src;
    g_srcSize = size;
    g_index = 0;
    g_initSend = false;
    g_pos = 0;
    g_blockMaxSize = blockMaxSize;
    g_blockCommandInit[BLOCK_SIZE_INDEX] = blockMaxSize;
}

uint16_t blockCommand_cb()
{
    uint16_t ret = g_initSend ? g_blockCommandContent[g_index] : g_blockCommandInit[g_index];
    

    g_index++;
    if (g_index == 8)
    {
        g_index = 0;
        g_initSend = true;
    } 

    return ret;
}

