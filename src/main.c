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

#include "link_defines.h"

typedef uint16_t (*TransmitHandler)();
static TransmitHandler g_transmitHandler = NULL;

static uint16_t g_receiveCommand[8] = {};
static struct k_sem g_command_semaphore;

static uint16_t crc_cb(uint16_t rx_bytes);

#define CMD_INDEX 0

static uint8_t g_counter = 0;

bool runTransitionDoneCb = false;

uint16_t playerRawBytes[0x1E] = {0x6147, 0x656D, 0x7246, 0x6165, 0x206B, 0x6E69, 0x2E63, 0x0000, 0x4003, 0x8000, 0x529E, 0x1805, 0xE3C4, 0xE0D9, 0x00FF, 0x0000, 0x0000, 0x0000, 0x1133, 0x0000, 0x0000, 0x0005, 0x6147, 0x656D, 0x7246, 0x6165, 0x206B, 0x6E69, 0x2E63, 0x0000};
uint16_t trainerCardRawBytes[0x32] = { 
    0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0003,
    0x529E, 0x0001, 0x0005, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x248C, 0x0000, 0x0A29,
    0x1413, 0x1037, 0x020E, 0xE3C4, 0xE0D9, 0x00FF, 0x0000,
    0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x4200, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000
};
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
    return rx_bytes;
    // runTransitionDoneCb = false;   
    // if (rx_bytes == LINK_MASTER_HANDSHAKE) link_setTransiveCallback(&crc_cb);
    // return LINK_SLAVE_HANDSHAKE;
}

uint16_t crc_cb(uint16_t rx_bytes)
{
    runTransitionDoneCb = false;
    link_setTransiveCallback(command_cb);
    if (rx_bytes == 0x8000) return 0x00;
    return rx_bytes;
}

bool send = false;
bool sendCard = false;
bool sendMovement = false;
uint16_t g_cafeCounter = 0;

int main(void)
{
    link_setTransiveCallback(&handshake_cb);
    link_setTransiveDoneCallback(&transiveDone_cb);
    k_sem_init(&g_command_semaphore, 0, 1);

    while (1)
    {
        k_sem_take(&g_command_semaphore, K_FOREVER);

        if (send)
        {
            const struct LinkPlayerBlock* linkPlayerBlock = linkPLayer();
            blockCommandSetup(playerRawBytes, sizeof(playerRawBytes), 3, sizeof(playerRawBytes));
            g_transmitHandler = blockCommand_cb;
            send = false;
        }
        else if (sendCard)
        {
            const struct TrainerCard* trainerCard = trainerCardPlaceholder();
            blockCommandSetup(trainerCardRawBytes, sizeof(trainerCardRawBytes), 3, 0x64);
            g_transmitHandler = blockCommand_cb;
            g_cafeCounter = 0;
            sendCard = false;
        }
        else if (sendMovement)
        {
            g_transmitHandler = moveCommand_cb;
        } 
        else
        {
            switch(g_receiveCommand[CMD_INDEX])
            {
                case LINKCMD_READY_CLOSE_LINK:
                    link_setTransiveCallback(&handshake_cb);
                    break;
                
                case LINKCMD_SEND_LINK_TYPE:
                    if (g_receiveCommand[1] == LINKTYPE_TRADE_SETUP)
                    {
                        g_transmitHandler = emptyCommand_cb;
                        send = true;
                    }
                    break;
                case LINKCMD_SEND_BLOCK_REQ:
                    g_transmitHandler = emptyCommand_cb;
                    sendCard = true;
                    break;
                case 0x00:
                    g_cafeCounter++;
                    if (g_cafeCounter > 328 && !sendMovement)
                    {
                        sendMovement = true;
                    }
                    break;
                    
            }
        }
        if (g_transmitHandler == blockCommand_cb)
        {
            blockCommandChunk();
        }
        
    }

    return 0;
}