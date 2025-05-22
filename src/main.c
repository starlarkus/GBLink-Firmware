#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>

#include "./link/link.h"

#include "./payloads/linkPlayer.h"
#include "./payloads/trainerCard.h"

#include "./callbacks/blockCommand.h"
#include "./callbacks/emptyCommand.h"
#include "./callbacks/moveCommand.h"
#include "./callbacks/blockRequestCommand.h"
#include "./callbacks/readyExitStandbyCommand.h"

#include "link_defines.h"

typedef uint16_t (*TransmitHandler)();
static TransmitHandler g_transmitHandler = NULL;

static uint16_t g_receiveCommand[8] = {};
static struct k_sem g_command_semaphore;

static uint16_t crc_cb(uint16_t rx_bytes);

#define CMD_INDEX 0

bool blockRequest = false;

static uint8_t g_counter = 0;

bool runTransitionDoneCb = false;

void transiveDone_cb()
{
    if (!runTransitionDoneCb) return;
    g_counter++;

    if (g_counter == 8)
    {
        g_counter = 0;
        link_setTransiveCallback(crc_cb);
        k_sem_give(&g_command_semaphore);
    }
}

uint16_t command_cb(uint16_t rx_bytes)
{
    runTransitionDoneCb = true;
    g_receiveCommand[g_counter] = rx_bytes;
    
    if (g_transmitHandler == NULL) return 0x00;
    return g_transmitHandler();
}

uint16_t handshake_cb(uint16_t rx_bytes)
{
    runTransitionDoneCb = false;
    if (rx_bytes == LINK_MASTER_HANDSHAKE) 
    {
        link_setTransiveCallback(&crc_cb);
        return LINK_MASTER_HANDSHAKE;
    }
    
    return LINK_SLAVE_HANDSHAKE;
}

uint16_t crc_cb(uint16_t rx_bytes)
{
    runTransitionDoneCb = false;
    link_setTransiveCallback(command_cb);
    return rx_bytes;
}

int countTranmissions = false;
int counter = 0;
uint16_t mirror_cb(uint16_t rx_byte) {
    if (countTranmissions) counter++;
    if (counter >= 30) return 0x00;
    return rx_byte; 
}

bool send = false;
bool sendCard = false;
bool sendMovement = false;
bool chunkingDone = false;
uint16_t mirrorCounter = 0;

int main(void)
{
    link_setTransiveCallback(&handshake_cb);
    link_setTransiveDoneCallback(&transiveDone_cb);
    k_sem_init(&g_command_semaphore, 0, 1);

    while (1)
    {
        k_sem_take(&g_command_semaphore, K_FOREVER);
        if (chunkingDone)
        {
            chunkingDone = false;
            g_transmitHandler = emptyCommand_cb;
        }

        if (g_transmitHandler == readyExitStandbyCommand_cb)
        {
            g_transmitHandler = emptyCommand_cb;
        }

        if (g_transmitHandler == moveCommand_cb)
        {
            g_transmitHandler = emptyCommand_cb;
        }

        switch(g_receiveCommand[CMD_INDEX])
        {
            case LINKCMD_READY_CLOSE_LINK:
                link_setTransiveCallback(&handshake_cb);
                break;
            
            case LINKCMD_SEND_LINK_TYPE:
                if (g_receiveCommand[1] == LINKTYPE_TRADE_SETUP)
                {
                    const struct LinkPlayerBlock* linkPlayerBlock = linkPLayer();
                    blockCommandSetup(linkPlayerBlock, sizeof(*linkPlayerBlock), sizeof(*linkPlayerBlock));
                    g_transmitHandler = blockCommand_cb;
                }
                break;
        
            case LINKCMD_SEND_BLOCK_REQ:
                const struct TrainerCard* trainerCard = trainerCardPlaceholder();
                blockCommandSetup(trainerCard, sizeof(*trainerCard), 0x64);
                g_transmitHandler = blockCommand_cb;
                break;
            
            case LINKCMD_READY_EXIT_STANDBY:
                g_transmitHandler = readyExitStandbyCommand_cb;
                break;
            
            case LINKCMD_SEND_HELD_KEYS:
                g_transmitHandler = moveCommand_cb;
                break;
        }
        
        if (g_transmitHandler == blockCommand_cb)
        {
            chunkingDone = blockCommandChunk();
        }
        
    }

    return 0;
}